#include "driver.h"
#include "vidhrdw/generic.h"

#define NAMCO_S1_RAM_REGION 5
#define NAMCOS1_MAX_BANK 0x400

/* #define NAMCOS1_OPCODE_COPY */

/* from vidhrdw */
extern int namcos1_videoram_r( int offset );
extern void namcos1_videoram_w( int offset, int data );
extern int namcos1_paletteram_r( int offset );
extern void namcos1_paletteram_w( int offset, int data );
extern void namcos1_videocontroll_w(int offset,int data);
extern void namcos1_set_scroll_offsets( const int *bgx, const int*bgy, int negative, int optimize );
extern void namcos1_set_optimize( int optimize );
extern void namcos1_set_sprite_offsets( int x, int y );

#define NAMCOS1_MAX_KEY 0x100
static unsigned char key[NAMCOS1_MAX_KEY];

static unsigned char *s1ram;

static int namcos1_cpu1_banklatch;

/**************************************************************************************
*	                                                                                  *
*	Key emulation (CUS136) Rev1 (Pacmania & Galaga 88)                                *
*	                                                                                  *
**************************************************************************************/

static int key_id;
static int key_id_query;

static int rev1_key_r( int offset ) {
	//if (errorlog) fprintf(errorlog,"CPU #%d PC %08x: keychip read %04X=%02x\n",cpu_getactivecpu(),cpu_get_pc(),offset,key[offset]);
	if(offset >= NAMCOS1_MAX_KEY)
	{
		if(errorlog) fprintf(errorlog,"CPU #%d PC %08x: unmapped keychip read %04x\n",cpu_getactivecpu(),cpu_get_pc(),offset);
		return 0;
	}
	return key[offset];
}

static void rev1_key_w( int offset, int data ) {
	static unsigned short divider, divide_32 = 0;
	//if(errorlog) fprintf(errorlog,"CPU #%d PC %08x: keychip write %04X=%02x\n",cpu_getactivecpu(),cpu_get_pc(),offset,data);
	if(offset >= NAMCOS1_MAX_KEY)
	{
		if(errorlog) fprintf(errorlog,"CPU #%d PC %08x: unmapped keychip write %04x=%04x\n",cpu_getactivecpu(),cpu_get_pc(),offset,data);
		return;
	}

	key[offset] = data;

	switch ( offset )
	{
	case 0x01:
		divider = ( key[0] << 8 ) + key[1];
		break;
	case 0x03:
		{
			static unsigned short d;
			unsigned short	v1, v2;
			unsigned long	l=0;

			if ( divide_32 )
				l = d << 16;

			d = ( key[2] << 8 ) + key[3];

			if ( divider == 0 ) {
				v1 = 0xffff;
				v2 = 0;
			} else {
				if ( divide_32 ) {
					l |= d;

					v1 = l / divider;
					v2 = l % divider;
				} else {
					v1 = d / divider;
					v2 = d % divider;
				}
			}

			key[2] = v1 >> 8;
			key[3] = v1;
			key[0] = v2 >> 8;
			key[1] = v2;
		}
		break;
	case 0x04:
		if ( key[4] == key_id_query ) /* get key number */
			key[4] = key_id;

		if ( key[4] == 0x0c )
			divide_32 = 1;
		else
			divide_32 = 0;
		break;
	}
}

/**************************************************************************************
*	                                                                                  *
*	Key emulation (CUS136) Rev2 (Dragon , Blazer , woeldcourt)                       *
*	                                                                                  *
**************************************************************************************/

static int rev2_key_r( int offset )
{
	//if(errorlog) fprintf(errorlog,"CPU #%d PC %08x: keychip read %04X=%02x\n",cpu_getactivecpu(),cpu_get_pc(),offset,key[offset]);
	if(offset >= NAMCOS1_MAX_KEY)
	{
		if(errorlog) fprintf(errorlog,"CPU #%d PC %08x: unmapped keychip read %04x\n",cpu_getactivecpu(),cpu_get_pc(),offset);
		return 0;
	}
	return key[offset];
}

static void rev2_key_w( int offset, int data )
{
	//if(errorlog) fprintf(errorlog,"CPU #%d PC %08x: keychip write %04X=%02x\n",cpu_getactivecpu(),cpu_get_pc(),offset,data);
	if(offset >= NAMCOS1_MAX_KEY)
	{
		if(errorlog) fprintf(errorlog,"CPU #%d PC %08x: unmapped keychip write %04x=%04x\n",cpu_getactivecpu(),cpu_get_pc(),offset,data);
		return;
	}
	key[offset] = data;

	switch(offset)
	{
	case 0x00:
		if ( data == 1 )
		{
			/* fetch key ID */
			key[3] = key_id;
			return;
		}
		break;
	case 0x02:
		/* $f2 = Dragon Spirit, $b7 = Blazer , $35($d9) = worldcourt */
		if ( key[3] == 0xf2 || key[3] == 0xb7 || key[3] == 0x35 )
		{
			switch( key[0] )
			{
				case 0x10: key[0] = 0x05; key[1] = 0x00; key[2] = 0xc6; break;
				case 0x12: key[0] = 0x09; key[1] = 0x00; key[2] = 0x96; break;
				case 0x15: key[0] = 0x0a; key[1] = 0x00; key[2] = 0x8f; break;
				case 0x22: key[0] = 0x14; key[1] = 0x00; key[2] = 0x39; break;
				case 0x32: key[0] = 0x31; key[1] = 0x00; key[2] = 0x12; break;
				case 0x3d: key[0] = 0x35; key[1] = 0x00; key[2] = 0x27; break;
				case 0x54: key[0] = 0x10; key[1] = 0x00; key[2] = 0x03; break;
				case 0x58: key[0] = 0x49; key[1] = 0x00; key[2] = 0x23; break;
				case 0x7b: key[0] = 0x48; key[1] = 0x00; key[2] = 0xd4; break;
				case 0xc7: key[0] = 0xbf; key[1] = 0x00; key[2] = 0xe8; break;
			}
			return;
		}
		break;
	case 0x03:
		/* $c2 = Dragon Spirit, $b6 = Blazer */
		if ( key[3] == 0xc2 || key[3] == 0xb6 ) {
			key[3] = 0x36;
			return;
		}
		/* $d9 = World court */
		if ( key[3] == 0xd9 )
		{
			key[3] = 0x35;
			return;
		}
		break;
	case 0x3f:	/* Splatter House */
		key[0x3f] = 0xb5;
		key[0x36] = 0xb5;
		return;
	}
	/* ?? */
	if ( key[3] == 0x01 ) {
		if ( key[0] == 0x40 && key[1] == 0x04 && key[2] == 0x00 ) {
			key[1] = 0x00; key[2] = 0x10;
			return;
		}
	}
}

/**************************************************************************************
*	                                                                                  *
*	Banking emulation (CUS117)                                                        *
*	                                                                                  *
**************************************************************************************/

#if 0
static int soundram_r( int offset)
{
	if(offset<0x100)
		return namcos1_wavedata_r(offset);
	if(offset<0x140)
		return namcos1_sound_r(offset-0x100);
	/* sahred ram */
	return namco_wavedata[offset];
}
#endif

static void soundram_w( int offset, int data )
{
	if(offset<0x100)
	{
		namcos1_wavedata_w(offset,data);
		return;
	}
	if(offset<0x140)
	{
		namcos1_sound_w(offset-0x100,data);
		return;
	}
	/* sahred ram */
	namco_wavedata[offset] = data;
}

/* ROM handlers */

static void rom_w( int offset, int data ) {
	if(errorlog)
		fprintf(errorlog,"CPU #%d PC %04x: warning - write %02x to rom address %04x\n",cpu_getactivecpu(),cpu_get_pc(),data,offset);
}

/* error handlers */
static int unknown_r( int offset ) {
	if(errorlog)
		fprintf(errorlog,"CPU #%d PC %04x: warning - read from unknown chip\n",cpu_getactivecpu(),cpu_get_pc() );
	return 0;
}

static void unknown_w( int offset, int data) {
	if(errorlog)
		fprintf(errorlog,"CPU #%d PC %04x: warning - wrote to unknown chip\n",cpu_getactivecpu(),cpu_get_pc() );
}

/* Bank handler definitions */
typedef int (*handler_r)(int offset);
typedef void (*handler_w)(int offset, int data);

typedef struct {
	handler_r	bank_handler_r;
	handler_w	bank_handler_w;
	int			bank_offset;
	unsigned char *bank_pointer;
} bankhandler;

static bankhandler namcos1_bank_element[NAMCOS1_MAX_BANK];

/* This is where we store our handlers */
/* 2 cpus with 8 banks of 8k each      */
static bankhandler		namcos1_banks[2][8];

/* Main bankswitching routine */
void namcos1_bankswitch_w( int offset, int data ) {
	static int chip = 0;

	if ( offset & 1 ) {
		int bank = ( offset >> 9 ) & 0x07; //0x0f;
		int cpu = cpu_getactivecpu();
		chip &= 0x0300;
		chip |= ( data & 0xff );
		/* copy bank handler */
		namcos1_banks[cpu][bank].bank_handler_r =  namcos1_bank_element[chip].bank_handler_r;
		namcos1_banks[cpu][bank].bank_handler_w =  namcos1_bank_element[chip].bank_handler_w;
		namcos1_banks[cpu][bank].bank_offset    =  namcos1_bank_element[chip].bank_offset;
		namcos1_banks[cpu][bank].bank_pointer   =  namcos1_bank_element[chip].bank_pointer;
		//memcpy( &namcos1_banks[cpu][bank] , &namcos1_bank_element[chip] , sizeof(bankhandler));
#if 1
		if( namcos1_banks[cpu][bank].bank_handler_r == unknown_r)
		{
			if (errorlog)
				fprintf(errorlog,"CPU %d : Unknown chip selected ($%04x) at PC %04x\n", cpu_getactivecpu(), chip, cpu_get_pc() );
		}
#endif
		if( namcos1_banks[cpu][bank].bank_handler_w == rom_w)
		/* Since we might be executing code, lets copy it to RAM */
		{
#ifdef NAMCOS1_OPCODE_COPY
			unsigned char *src,*dst;
			src = namcos1_banks[cpu][bank].bank_pointer;
			dst = Machine->memory_region[Machine->drv->cpu[cpu].memory_region] + (bank<<13);
			memcpy( dst, src, 0x2000 );
#endif
		}
		/* renew pc base */
//		change_pc16(cpu_get_pc());
	} else {
		chip &= 0x00ff;
		chip |= ( data & 0xff ) << 8;
	}
}

/* Sub cpu set start bank port */
void namcos1_subcpu_bank(int offset,int data)
{
	int oldcpu = cpu_getactivecpu();
	int chip   = ((namcos1_cpu1_banklatch<<8)|data)&0x3ff;
	//if(errorlog) fprintf(errorlog,"cpu1 bank selected %02x=%02x\n",offset,chip);
	/* Prepare code for Cpu 1 */
	cpu_setactivecpu( 1 );
	namcos1_bankswitch_w( 0x0e00, chip>>8  );
	namcos1_bankswitch_w( 0x0e01, chip&0xff);
	cpu_reset( 1 );
	cpu_setactivecpu( oldcpu );
}

#define MR_HANDLER(cpu,bank) \
int namcos1_##cpu##_banked_area##bank##_r(int offset) {\
	if( namcos1_banks[cpu][bank].bank_handler_r) \
		return (*namcos1_banks[cpu][bank].bank_handler_r)( offset+namcos1_banks[cpu][bank].bank_offset); \
	return namcos1_banks[cpu][bank].bank_pointer[offset]; }

MR_HANDLER(0,0)
MR_HANDLER(0,1)
MR_HANDLER(0,2)
MR_HANDLER(0,3)
MR_HANDLER(0,4)
MR_HANDLER(0,5)
MR_HANDLER(0,6)
MR_HANDLER(0,7)
MR_HANDLER(1,0)
MR_HANDLER(1,1)
MR_HANDLER(1,2)
MR_HANDLER(1,3)
MR_HANDLER(1,4)
MR_HANDLER(1,5)
MR_HANDLER(1,6)
MR_HANDLER(1,7)
#undef MR_HANDLER

#define MW_HANDLER(cpu,bank) \
void namcos1_##cpu##_banked_area##bank##_w( int offset, int data ) {\
	if( namcos1_banks[cpu][bank].bank_handler_w) \
	{ \
		(*namcos1_banks[cpu][bank].bank_handler_w)( offset+ namcos1_banks[cpu][bank].bank_offset,data ); \
		return; \
	}\
	namcos1_banks[cpu][bank].bank_pointer[offset]=data; \
}

MW_HANDLER(0,0)
MW_HANDLER(0,1)
MW_HANDLER(0,2)
MW_HANDLER(0,3)
MW_HANDLER(0,4)
MW_HANDLER(0,5)
MW_HANDLER(0,6)
MW_HANDLER(1,0)
MW_HANDLER(1,1)
MW_HANDLER(1,2)
MW_HANDLER(1,3)
MW_HANDLER(1,4)
MW_HANDLER(1,5)
MW_HANDLER(1,6)
#undef MW_HANDLER

/**************************************************************************************
*	                                                                                  *
*	63701 MCU emulation (CUS64)                                                       *
*	                                                                                  *
**************************************************************************************/

void namcos1_cpu_control_w( int offset, int data )
{
	//if(errorlog) fprintf(errorlog,"reset controll %02x\n",data);
	if ( data & 1 ) {
		//cpu_reset( 1 );
		cpu_reset( 2 );
		cpu_reset( 3 );
		cpu_halt( 1, 1 );
		cpu_halt( 2, 1 );
		cpu_halt( 3, 1 );
	} else {
		cpu_halt( 1, 0 );
		cpu_halt( 2, 0 );
		cpu_halt( 3, 0 );
	}
}

/**************************************************************************************
*	                                                                                  *
*	Sound banking emulation (CUS121)                                                  *
*	                                                                                  *
**************************************************************************************/

void namcos1_sound_bankswitch_w( int offset, int data )
{
	unsigned char *RAM = Machine->memory_region[3];
	int bank = ( data >> 4 ) & 0x07;

	cpu_setbank( 1, &RAM[ 0x0c000 + ( 0x4000 * bank ) ] );
}

/**************************************************************************************
*	                                                                                  *
*	Initialization                                                                    *
*	                                                                                  *
**************************************************************************************/


static int namcos1_setopbase (int pc) {
#ifdef NAMCOS1_OPCODE_COPY
	return -1;
#else
	int cpu  = cpu_getactivecpu();
	if( cpu < 2 )
	{
		int bank = (pc>>13)&7;
		/* cpu 0 ot cpu1 */
		OP_RAM = OP_ROM = (namcos1_banks[cpu][bank].bank_pointer) - (bank<<13);
		/* memory.c output warning - op-code execute on mapped i/o */
		/* but,It is neesarry to continue cpu_setOPbase16 function */
		/* for update current operationhardware(ophw) code         */
		return pc;
	}
	/* don't care other cpus */
	return pc;
#endif
}

static void namcos1_insatll_bank(int start,int end,handler_r hr,handler_w hw,
                          int offset,unsigned char *pointer)
{
	int i;
	for(i=start;i<=end;i++)
	{
		namcos1_bank_element[i].bank_handler_r = hr;
		namcos1_bank_element[i].bank_handler_w = hw;
		namcos1_bank_element[i].bank_offset    = offset;
		namcos1_bank_element[i].bank_pointer   = pointer;
		offset  += 0x2000;
		if(pointer) pointer += 0x2000;
	}
}

static void namcos1_install_rom_bank(int start,int end,int size,int offset)
{
	unsigned char *BROM = Machine->memory_region[4];
	int step = size/0x2000;
	while(start < end)
	{
		namcos1_insatll_bank(start,start+step-1,0,rom_w,0,&BROM[offset]);
		start += step;
	}
}

static void namcos1_build_banks(/* int *romsize_maps,*/
                           handler_r key_r,handler_w key_w)
{
	int i;

	/* S1 RAM pointer set */
	s1ram = Machine->memory_region[NAMCO_S1_RAM_REGION];

	/* clear all banks to unknown area */
	for(i=0;i<NAMCOS1_MAX_BANK;i++)
		namcos1_insatll_bank(i,i,unknown_r,unknown_w,0,0);

	/* palette sprites / palette chars / chars? */
	namcos1_insatll_bank(0x170,0x172,namcos1_paletteram_r,namcos1_paletteram_w,0,s1ram);
	/* work ram */
	namcos1_insatll_bank(0x173,0x173,0,0,0x6000,&s1ram[0x6000]);
	/* RAM 5 banks - Videoram */
	namcos1_insatll_bank(0x178,0x17b,namcos1_videoram_r,namcos1_videoram_w,0,0);
	/* key chip bank (rev1_key_w / rev2_key_w ) */
	namcos1_insatll_bank(0x17c,0x17c,key_r,key_w,0,0);
	/* namcos1_insatll_bank(0x17c,0x17c,0,key_w,0,key); */
	/* RAM 1 banks display controll , playfields , sprite */
	namcos1_insatll_bank(0x17e,0x17e,0,namcos1_videocontroll_w,0,&s1ram[0x8000]);
	/* RAM 1 shared ram , PSG device */
	/* namcos1_insatll_bank(0x17f,0x17f,soundram_r,soundram_w,0x0000,namco_wavedata); */
	namcos1_insatll_bank(0x17f,0x17f,0,soundram_w,0x0000,namco_wavedata);
	/* RAM3 */
	namcos1_insatll_bank(0x180,0x183,0,0,0,&s1ram[0xc000]);
	/* PRG0 */
	namcos1_install_rom_bank(0x200,0x23f,0x20000 , 0xe0000);
	/* PRG1 */
	namcos1_install_rom_bank(0x240,0x27f,0x20000 , 0xc0000);
	/* PRG2 */
	namcos1_install_rom_bank(0x280,0x2bf,0x20000 , 0xa0000);
	/* PRG3 */
	namcos1_install_rom_bank(0x2c0,0x2ff,0x20000 , 0x80000);
	/* PRG4 */
	namcos1_install_rom_bank(0x300,0x33f,0x20000 , 0x60000);
	/* PRG5 */
	namcos1_install_rom_bank(0x340,0x37f,0x20000 , 0x40000);
	/* PRG6 */
	namcos1_install_rom_bank(0x380,0x3bf,0x20000 , 0x20000);
	/* PRG7 */
	namcos1_install_rom_bank(0x3c0,0x3ff,0x20000 , 0x00000);
}

/* extern int m6809_slapstic; */

void namcos1_machine_init( void ) {

	int oldcpu = cpu_getactivecpu(), i;

	/* Point all of our bankhandlers to the error handlers */
	for ( i = 0; i < 8; i++ ) {
		namcos1_banks[0][i].bank_handler_r = unknown_r;
		namcos1_banks[0][i].bank_handler_w = unknown_w;
		namcos1_banks[0][i].bank_offset = 0;
		namcos1_banks[1][i].bank_handler_r = unknown_r;
		namcos1_banks[1][i].bank_handler_w = unknown_w;
		namcos1_banks[1][i].bank_offset = 0;
	}

	/* Prepare code for Cpu 0 */
	cpu_setactivecpu( 0 );
	namcos1_bankswitch_w( 0x0e00, 0x03 ); /* bank7 = 0x3ff(PRG7) */
	namcos1_bankswitch_w( 0x0e01, 0xff );
#if 0
	/* Prepare code for Cpu 1 */
	cpu_setactivecpu( 1 );
	namcos1_bankswitch_w( 0x0e00, 0x03);
	namcos1_bankswitch_w( 0x0e01, 0xff);
#endif
	namcos1_cpu1_banklatch = 0x03;

	/* reset starting Cpu */
	cpu_setactivecpu( oldcpu );

	/* Point mcu & sound shared RAM to destination */
	{
		unsigned char *RAM = namco_wavedata + 0x1000; /* Ram 1, bank 1, offset 0x1000 */
		cpu_setbank( 2, RAM );
		cpu_setbank( 3, RAM );
	}



	/* In case we had some cpu's suspended, resume them now */
	//cpu_halt( 1, 1 );
	//cpu_halt( 2, 1 );
	//cpu_halt( 3, 1 );

	cpu_halt( 1, 0 );
	cpu_halt( 2, 0 );
	cpu_halt( 3, 0 );
}
/**************************************************************************************
*	                                                                                  *
*	driver specific initialize routine
*	                                                                                  *
**************************************************************************************/
struct namcos1_specific
{
	/* keychip */
	int key_id_query , key_id;
	handler_r key_r;
	handler_w key_w;
	/* optimiaze flag , use tilemap for playfield */
	int tilemap_use;
	/* start bank number */
//	int cpu0_start_bank , cpu1_start_bank;
};

static void namcos1_driver_init(const struct namcos1_specific *specific )
{
	/* keychip id */
	key_id_query = specific->key_id_query;
	key_id       = specific->key_id;

	/* tilemap use optimize option */
	namcos1_set_optimize( specific->tilemap_use );

	/* build bank elements */
	namcos1_build_banks(specific->key_r,specific->key_w);

	/* override opcode handling for extended memory bank handler */
	/* m6809_slapstic = 1; */
	cpu_setOPbaseoverride( namcos1_setopbase );
}

/**************************************************************************************
*	Shadowland / Youkai Douchuuki specific                                            *
**************************************************************************************/

void shadowld_driver_init( void )
{
	const struct namcos1_specific shadowld_specific=
	{
		0x00,0x00,							/* key query , key id */
		rev1_key_r,rev1_key_w,				/* key handler */
		1									/* use tilemap flag : speedup optimize */
//		0x03ff,0x03fb						/* start bank   */
	};
	namcos1_driver_init(&shadowld_specific);
}

/**************************************************************************************
*	Dragon Spirit specific                                                           *
**************************************************************************************/

/* Theres is an id check followed by some key nightmare */
void dspirit_driver_init( void )
{
	const struct namcos1_specific dspirit_specific=
	{
		0x00,0x36,							/* key query , key id */
		rev2_key_r,rev2_key_w,				/* key handler */
		1									/* use tilemap flag : speedup optimize */
//		0x03ff,0x03fb						/* start bank   */
	};
	namcos1_driver_init(&dspirit_specific);
}

#if 0
/**************************************************************************************
*	Quester specific                                                           *
**************************************************************************************/

/* Theres is an id check followed by some key nightmare */
void quester_driver_init( void )
{
	const struct namcos1_specific quester_specific=
	{
		0x00,0x00,							/* key query , key id */
		rev1_key_r,rev1_key_w,				/* key handler */
		1									/* use tilemap flag : speedup optimize */
//		0x03ff,0x03fb						/* start bank   */
	};
	namcos1_driver_init(&quester_specific);
}
#endif

/**************************************************************************************
*	Blazer specific			                                                          *
**************************************************************************************/

/* Theres is an id check followed by some key nightmare */

void blazer_driver_init( void )
{
	const struct namcos1_specific blazer_specific=
	{
		0x00,0x13,							/* key query , key id */
		rev2_key_r,rev2_key_w,				/* key handler */
		1									/* use tilemap flag : speedup optimize */
//		0x03ff,0x03fb						/* start bank   */
	};
	namcos1_driver_init(&blazer_specific);
}

/**************************************************************************************
*	Pacmania / Pacmania (japan) specific                                                                 *
**************************************************************************************/
void pacmania_driver_init( void )
{
	const struct namcos1_specific pacmania_specific=
	{
		0x4b,0x12,							/* key query , key id */
		rev1_key_r,rev1_key_w,				/* key handler */
		1									/* use tilemap flag : speedup optimize */
//		0x03ff,0x03fb						/* start bank   */
	};
	namcos1_driver_init(&pacmania_specific);
}

/**************************************************************************************
*	Galaga 88 / Galaga 88 (japan) specific                                                                 *
**************************************************************************************/
void galaga88_driver_init( void )
{
	const struct namcos1_specific galaga88_specific=
	{
		0x2d,0x31,							/* key query , key id */
		rev1_key_r,rev1_key_w,				/* key handler */
		1									/* use tilemap flag : speedup optimize */
//		0x03ff,0x03fb						/* start bank   */
	};
	namcos1_driver_init(&galaga88_specific);
}

#if 0
/**************************************************************************************
*	World Stadium specific                                                                 *
**************************************************************************************/
void wstadium_driver_init( void )
{
	const struct namcos1_specifi wstadium_specific=
	{
		0x00,0x00,							/* key query , key id */
		rev1_key_r,rev1_key_w,				/* key handler */
		1									/* use tilemap flag : speedup optimize */
//		0x03ff,0x03fb						/* start bank   */
	};
	namcos1_driver_init(&wstadium_specific);
}
#endif

/**************************************************************************************
*	Berabohman specific                                                                 *
**************************************************************************************/
void berabohm_driver_init( void )
{
	const struct namcos1_specific berabohm_specific=
	{
		0x00,0x00,							/* key query , key id */
		rev1_key_r,rev1_key_w,				/* key handler */
		1									/* use tilemap flag : speedup optimize */
//		0x03ff,0x03fb						/* start bank   */
	};
	namcos1_driver_init(&berabohm_specific);
}

/**************************************************************************************
*	Alice in Wonderland / Merhen Maze specific                                        *
**************************************************************************************/
void alice_driver_init( void )
{
	const struct namcos1_specific alice_specific=
	{
		0x5b,0x25,							/* key query , key id */
		rev1_key_r,rev1_key_w,				/* key handler */
		1									/* use tilemap flag : speedup optimize */
//		0x03ff,0x03ef						/* start bank   */
	};
	namcos1_driver_init(&alice_specific);
}

/**************************************************************************************
*	Bakutotsu Kijuutei specific                                                                 *
**************************************************************************************/
void bakutotu_driver_init( void )
{
	const struct namcos1_specific bakutotu_specific=
	{
		0x03,0x22,							/* key query , key id */
		rev1_key_r,rev1_key_w,				/* key handler */
		1									/* use tilemap flag : speedup optimize */
//		0x03ff,0x03fe						/* start bank   */
	};
	namcos1_driver_init(&bakutotu_specific);
}

/**************************************************************************************
*	World Court specific                                                                 *
**************************************************************************************/
void wldcourt_driver_init( void )
{
	const struct namcos1_specific worldcourt_specific=
	{
		0x00,0x35,							/* key query , key id */
		rev2_key_r,rev2_key_w,				/* key handler */
		1									/* use tilemap flag : speedup optimize */
//		0x03ff,0x03f8						/* start bank   */
	};
	namcos1_driver_init(&worldcourt_specific);
}

/**************************************************************************************
*	Splatter House specific                                                           *
**************************************************************************************/

/* Theres is an id check followed by some key nightmare */

void splatter_driver_init( void )
{
	const struct namcos1_specific splatter_specific=
	{
		0x00,0x00,							/* key query , key id */
		rev2_key_r,rev2_key_w,				/* key handler */
		1									/* use tilemap flag : speedup optimize */
//		0x03ff,0x037f						/* start bank   */
	};
	namcos1_driver_init(&splatter_specific);
}

#if 0
/**************************************************************************************
*	Face Off specific                                                           *
**************************************************************************************/
void faceoff_driver_init( void )
{
	const struct namcos1_specific faceoff_specific=
	{
		0x00,0x00,							/* key query , key id */
		rev1_key_r,rev1_key_w,				/* key handler */
		1									/* use tilemap flag : speedup optimize */
//		0x03ff,0x03fb						/* start bank   */
	};
	namcos1_driver_init(&faceoff_specific);
}
#endif

/**************************************************************************************
*	Rompers specific                                                           *
**************************************************************************************/
void rompers_driver_init( void )
{
	const struct namcos1_specific rompers_specific=
	{
		0x00,0x00,							/* key query , key id */
		rev1_key_r,rev1_key_w,				/* key handler */
		1									/* use tilemap flag : speedup optimize */
//		0x03ff,0x03fd						/* start bank   */
	};
	namcos1_driver_init(&rompers_specific);
	key[0x70] = 0xb6;
}

/**************************************************************************************
*	Blast Off specific                                                           *
**************************************************************************************/
void blastoff_driver_init( void )
{
	const struct namcos1_specific blastoff_specific=
	{
		0x00,0x00,							/* key query , key id */
		rev1_key_r,rev1_key_w,				/* key handler */
		1									/* use tilemap flag : speedup optimize */
//		0x03ff,0x03f7						/* start bank   */
	};
	namcos1_driver_init(&blastoff_specific);
	key[0] = 0xb7;
}

#if 0
/**************************************************************************************
*	World Stadium 89 specific                                                       *
**************************************************************************************/
void ws89_driver_init( void )
{
	const struct namcos1_specific ws89_specific=
	{
		0x00,0x00,							/* key query , key id */
		rev1_key_r,rev1_key_w,				/* key handler */
		1									/* use tilemap flag : speedup optimize */
//		0x03ff,0x03fb						/* start bank   */
	};
	namcos1_driver_init(&ws89_specific);
}
#endif

/**************************************************************************************
*	Dangerous Seed specific                                                           *
**************************************************************************************/
void dangseed_driver_init( void )
{
	const struct namcos1_specific dangseed_specific=
	{
		0x00,0x00,							/* key query , key id */
		rev1_key_r,rev1_key_w,				/* key handler */
		1									/* use tilemap flag : speedup optimize */
//		0x03ff,0x03fb						/* start bank   */
	};
	namcos1_driver_init(&dangseed_specific);
	//key[0x67] = ;
	//key[0x57] = ;
	key[0x03] = 0x34;
}

/**************************************************************************************
*	World Stadium 90 specific                                                         *
**************************************************************************************/
/* Theres is an id check followed by some key nightmare */

void ws90_driver_init( void )
{
	const struct namcos1_specific ws90_specific=
	{
		0x00,0x00,							/* key query , key id */
		rev1_key_r,rev1_key_w,				/* key handler */
		1									/* use tilemap flag : speedup optimize */
//		0x03ff,0x03fb						/* start bank   */
	};
	namcos1_driver_init(&ws90_specific);

	key[0x47] = 0x36;
	key[0x40] = 0x36;
}

/**************************************************************************************
*	Pistol Daimyo no Bouken specific                                                         *
**************************************************************************************/
void pistoldm_driver_init( void )
{
	const struct namcos1_specific pistoldm_specific=
	{
		0x00,0x00,							/* key query , key id */
		rev1_key_r,rev1_key_w,				/* key handler */
		1									/* use tilemap flag : speedup optimize */
//		0x03ff,0x03f1						/* start bank   */
	};
	namcos1_driver_init(&pistoldm_specific);
	//key[0x17] = ;
	//key[0x07] = ;
	key[0x43] = 0x35;
}

/**************************************************************************************
*	PSouko Ban DX specific                                                         *
**************************************************************************************/
void soukobdx_driver_init( void )
{
	const struct namcos1_specific soukobdx_specific=
	{
		0x00,0x00,							/* key query , key id */
		rev1_key_r,rev1_key_w,				/* key handler */
		1									/* use tilemap flag : speedup optimize */
//		0x03ff,0x03fb						/* start bank   */
	};
	namcos1_driver_init(&soukobdx_specific);
	//key[0x27] = ;
	//key[0x07] = ;
	key[0x43] = 0x37;
}

/**************************************************************************************
*	Tank Force specific                                                         *
**************************************************************************************/
void tankfrce_driver_init( void )
{
	const struct namcos1_specific tankfrce_specific=
	{
		0x00,0x00,							/* key query , key id */
		rev1_key_r,rev1_key_w,				/* key handler */
		1									/* use tilemap flag : speedup optimize */
//		0x03ff,0x03fb						/* start bank   */
	};
	namcos1_driver_init(&tankfrce_specific);
	//key[0x57] = ;
	//key[0x17] = ;
	key[0x2b] = 0xb9;
}
