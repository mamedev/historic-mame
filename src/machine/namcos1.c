#include "driver.h"
#include "vidhrdw/generic.h"

#define NAMCO_S1_RAM_REGION 5
#define NAMCOS1_MAX_BANK 0x400

/* #define NAMCOS1_OPCODE_COPY */

/* from vidhrdw */
extern int namcos1_videoram_r( int offs );
extern void namcos1_videoram_w( int offs, int data );
extern void namcos1_playfield_control_w( int offs, int data );
extern void namcos1_set_scroll_offsets( int *bgx, int*bgy, int negative );
extern void namcos1_set_sprite_offsets( int x, int y );

static unsigned char key[0x2000];

static unsigned char *s1ram;

/**************************************************************************************
*	                                                                                  *
*	Key emulation (CUS136) Rev1 (Pacmania & Galaga 88)                                *
*	                                                                                  *
**************************************************************************************/

static int key_id;
static int key_id_query;

static int rev1_key_r( int offset ) {
	return key[offset];
}

static void rev1_key_w( int offset, int data ) {
	static unsigned short divider, divide_32 = 0;

	key[offset] = data;

	switch ( offset ) {

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
*	Key emulation (CUS136) Rev2 (Dragon Spirits and Blazer)                           *
*	                                                                                  *
**************************************************************************************/

static int rev2_key_r( int offset ) {
	return key[offset];
}

static void rev2_key_w( int offset, int data ) {
	key[offset] = data;

	if ( offset == 0 && data == 1 ) {
		/* fetch key ID */
		key[3] = key_id;
		return;
	}

	/* $f2 = Dragon Spirits, $b7 = Blazer */
	if ( offset == 2 && ( key[3] == 0xf2 || key[3] == 0xb7 ) ) {
		switch( key[0] ) {
			case 0x10:
				key[0] = 0x05; key[1] = 0x00; key[2] = 0xc6;
			break;

			case 0x12:
				key[0] = 0x09; key[1] = 0x00; key[2] = 0x96;
			break;

			case 0x15:
				key[0] = 0x0a; key[1] = 0x00; key[2] = 0x8f;
			break;

			case 0x22:
				key[0] = 0x14; key[1] = 0x00; key[2] = 0x39;
			break;

			case 0x32:
				key[0] = 0x31; key[1] = 0x00; key[2] = 0x12;
			break;

			case 0x3d:
				key[0] = 0x35; key[1] = 0x00; key[2] = 0x27;
			break;

			case 0x54:
				key[0] = 0x10; key[1] = 0x00; key[2] = 0x03;
			break;

			case 0x58:
				key[0] = 0x49; key[1] = 0x00; key[2] = 0x23;
			break;

			case 0x7b:
				key[0] = 0x48; key[1] = 0x00; key[2] = 0xd4;
			break;

			case 0xc7:
				key[0] = 0xbf; key[1] = 0x00; key[2] = 0xe8;
			break;
		}
		return;
	}

	if ( key[3] == 0x01 ) {
		if ( key[0] == 0x40 && key[1] == 0x04 && key[2] == 0x00 ) {
			key[1] = 0x00; key[2] = 0x10;
			return;
		}
	}

	/* $c2 = Dragon Spirits, $b6 = Blazer */
	if ( key[3] == 0xc2 || key[3] == 0xb6 ) {
		key[3] = 0x36;
		return;
	}

	/* Splatter House */
	if ( offset == 0x3f ) {
		key[0x3f] = 0xb5;
		key[0x36] = 0xb5;
	}
}

/**************************************************************************************
*	                                                                                  *
*	Banking emulation (CUS117)                                                        *
*	                                                                                  *
**************************************************************************************/

static void palette_w( int offset, int data)
{
	int page = offset & 0xe000;
	if(s1ram[ offset ] != data)
	{
		s1ram[ offset ] = data;
		if ( (offset&0x1fff) < 0x1800 ) {
			int color = ( offset & 0x7ff );
			int r = s1ram[ page + color ];
			int g = s1ram[ page + color + 0x800 ];
			int b = s1ram[ page + color + 0x1000 ];

			palette_change_color( color + ( page>>2 ), r, g, b );
		}
	}
}
static void ram1_w( int offset, int data )
{
	s1ram[ offset ] = data;
	if ( (offset&0x1fff) >= 0x1000 )
		namcos1_playfield_control_w( offset & 0xfff, data );
}

/* ROM handlers */

static void rom_w( int offset, int data ) {
	if ( errorlog )
		fprintf(errorlog,"CPU #%d PC %04x: warning - write %02x to rom address %04x\n",cpu_getactivecpu(),cpu_get_pc(),data,offset);
}

/* error handlers */
static int unknown_r( int offset ) {
	if ( errorlog )
		fprintf(errorlog,"CPU #%d PC %04x: warning - read from unknown chip\n",cpu_getactivecpu(),cpu_get_pc() );
	return 0;
}

static void unknown_w( int offset, int data) {
	if ( errorlog )
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
		int bank = ( offset >> 9 ) & 0x0f;
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
			if ( errorlog )
				fprintf( errorlog, "CPU %d : Unknown chip selected ($%04x) at PC %04x\n", cpu_getactivecpu(), chip, cpu_get_pc() );
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
		change_pc16(cpu_get_pc());
	} else {
		chip &= 0x00ff;
		chip |= ( data & 0xff ) << 8;
	}
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

void namcos1_cpu_control_w( int offset, int data ) {

	if ( data & 1 ) {
//		cpu_reset( 1 );
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

void namcos1_sound_bankswitch_w( int offset, int data ) {
	unsigned char *RAM = Machine->memory_region[3];
	int bank = ( data >> 4 ) & 0x07;

	cpu_setbank( 1, &RAM[ 0x10000 + ( 0x4000 * bank ) ] );
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
	return -1;
#endif
}

/* default values for cpu start banks */
static int cpu0_start;
static int cpu1_start;

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
                           handler_r key_r,handler_w key_w,
                           int cpu0_start_bank,int cpu1_start_bank)
{
	int i;

	/* S1 RAM pointer set */
	s1ram = Machine->memory_region[NAMCO_S1_RAM_REGION];

	/* set start bank */
	cpu0_start = cpu0_start_bank;
	cpu1_start = cpu1_start_bank;

	/* clear all banks to unknown area */
	for(i=0;i<NAMCOS1_MAX_BANK;i++)
		namcos1_insatll_bank(i,i,unknown_r,unknown_w,0,0);

	/* palette sprites / palette chars / chars? */
	namcos1_insatll_bank(0x170,0x172,0,palette_w,0,s1ram);
	/* rowk ram */
	namcos1_insatll_bank(0x173,0x173,0,0,0x6000,&s1ram[0x6000]);
	/* RAM 5 banks - Videoram */
	namcos1_insatll_bank(0x178,0x17b,namcos1_videoram_r,namcos1_videoram_w,0,0);
	/* key chip bank (rev1_key_w / rev2_key_w ) */
	/* namcos1_insatll_bank(0x17c,0x17c,key_r,key_w,0,0); */
	namcos1_insatll_bank(0x17c,0x17c,0,key_w,0,key);
	/* RAM 1 banks playfield */
	namcos1_insatll_bank(0x17e,0x17f,0,ram1_w,0x8000,&s1ram[0x8000]);
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

extern int m6809_slapstic;

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
	namcos1_bankswitch_w( 0x0e00, ( cpu0_start >> 8 ) & 0xff );
	namcos1_bankswitch_w( 0x0e01, ( cpu0_start & 0xff ) );

	/* Prepare code for Cpu 1 */
	cpu_setactivecpu( 1 );
	namcos1_bankswitch_w( 0x0e00, ( cpu1_start >> 8 ) & 0xff );
	namcos1_bankswitch_w( 0x0e01, ( cpu1_start & 0xff ) );

	/* reset starting Cpu */
	cpu_setactivecpu( oldcpu );

	/* Point sound shared RAM to destination */
	{
		unsigned char *RAM = ( Machine->memory_region[NAMCO_S1_RAM_REGION] ) + 0xa000; /* Ram 1, bank 1, offset 0x0000 */
		cpu_setbank( 2, RAM );
	}

	/* Point mcu & sound shared RAM to destination */
	{
		unsigned char *RAM = ( Machine->memory_region[NAMCO_S1_RAM_REGION] ) + 0xb000; /* Ram 1, bank 1, offset 0x1000 */
		cpu_setbank( 3, RAM );
		cpu_setbank( 4, RAM );
	}

	/* Set up Namco PSG wave address */
	namco_wavedata = ( Machine->memory_region[NAMCO_S1_RAM_REGION] ) + 0xa000; /* Ram 1, bank 1, offset 0x0000 */

	/* In case we had some cpu's suspended, resume them now */
	cpu_halt( 1, 1 );
	cpu_halt( 2, 1 );
	cpu_halt( 3, 1 );

	/* m6809_slapstic = 1; */
	cpu_setOPbaseoverride( namcos1_setopbase );
}

/**************************************************************************************
*	                                                                                  *
*	Alice in Wonderland specific                                                      *
*	                                                                                  *
**************************************************************************************/

void alice_driver_init( void ) {
	int alice_key_id = 0x25;
	int alice_key_id_query = 0x5b;

	/* Set Alice's key id query */
	key_id_query = alice_key_id_query;

	/* Set Alice's key id */
	key_id = alice_key_id;

	/* set scrolling offsets for this game */
	{
		int bgx[4] = { 0x0b0, 0x0b2, 0x0b3, 0x0b4 };
		int bgy[4] = { 0x108, 0x108, 0x108, 0x008 };

		namcos1_set_scroll_offsets( bgx, bgy, 0 );
	}

	/* set sprite offsets for this game */
	namcos1_set_sprite_offsets( 4, 204 );

	/* build bank elements */
	namcos1_build_banks(rev1_key_r,rev1_key_w,0x03ff,0x03bf);
}

/**************************************************************************************
*	                                                                                  *
*	Blazer specific			                                                          *
*	                                                                                  *
**************************************************************************************/

/* Theres is an id check followed by some key nightmare */

void blazer_driver_init( void ) {

	key_id = 0x13;

	/* set scrolling offsets for this game */
	{
		int bgx[4] = { 0x1d0, 0x1d2, 0x1d3, 0x1d4 };
		int bgy[4] = { 0x1e8, 0x1e8, 0x1e8, 0x0e8 };

		namcos1_set_scroll_offsets( bgx, bgy, 1 );
	}

	/* set sprite offsets for this game */
	namcos1_set_sprite_offsets( -120, 232 );

	/* build bank elements */
	namcos1_build_banks(rev2_key_r,rev2_key_w,0x03ff,0x03fb);
}

/**************************************************************************************
*	                                                                                  *
*	Dragon Spirits specific                                                           *
*	                                                                                  *
**************************************************************************************/

/* Theres is an id check followed by some key nightmare */

void dspirits_driver_init( void ) {

	key_id = 0x36;

	/* set scrolling offsets for this game */
	{
		int bgx[4] = { 0x1d0, 0x1d2, 0x1d3, 0x1d4 };
		int bgy[4] = { 0x1e8, 0x1e8, 0x1e8, 0x0e8 };

		namcos1_set_scroll_offsets( bgx, bgy, 1 );
	}

	/* set sprite offsets for this game */
	namcos1_set_sprite_offsets( -120, 232 );

	/* build bank elements */
	namcos1_build_banks(rev2_key_r,rev2_key_w,0x03ff,0x03fb);
}

/**************************************************************************************
*	                                                                                  *
*	Galaga 88 specific                                                                 *
*	                                                                                  *
**************************************************************************************/

void galaga88_driver_init( void ) {
	int galaga88_key_id = 0x31;
	int galaga88_key_id_query = 0x2d;

	/* Set Galaga 88's key id query */
	key_id_query = galaga88_key_id_query;

	/* Set Galaga 88's key id */
	key_id = galaga88_key_id;

	/* set scrolling offsets for this game */
	{
		int bgx[4] = { 0x1d0, 0x1d2, 0x1d3, 0x1d4 };
		int bgy[4] = { 0x1e8, 0x1e8, 0x1e8, 0x0e8 };

		namcos1_set_scroll_offsets( bgx, bgy, 1 );
	}

	/* set sprite offsets for this game */
	namcos1_set_sprite_offsets( -120, 232 );

	/* build bank elements */
	namcos1_build_banks(rev1_key_r,rev1_key_w,0x03ff,0x03fb);
}

/**************************************************************************************
*	                                                                                  *
*	Pacmania specific                                                                 *
*	                                                                                  *
**************************************************************************************/

void pacmania_driver_init( void ) {
	int pacmania_key_id = 0x12;
	int pacmania_key_id_query = 0x4b;

	/* Set Pacmania's key id query */
	key_id_query = pacmania_key_id_query;

	/* Set Pacmania's key id */
	key_id = pacmania_key_id;

	/* set scrolling offsets for this game */
	{
		int bgx[4] = { 0x0b0, 0x0b2, 0x0b3, 0x0b4 };
		int bgy[4] = { 0x108, 0x108, 0x108, 0x008 };

		namcos1_set_scroll_offsets( bgx, bgy, 0 );
	}

	/* set sprite offsets for this game */
	namcos1_set_sprite_offsets( -56, 240 );

	/* build bank elements */
	namcos1_build_banks(rev1_key_r,rev1_key_w,0x03ff,0x03fb);
}

/**************************************************************************************
*	                                                                                  *
*	Shadowland specific                                                               *
*	                                                                                  *
**************************************************************************************/

void shadowld_driver_init( void ) {
	/* Shadowland has no Key Chip - We fill up the handler just to avoid incosistencies */

	int shadowld_key_id = 0x00;
	int shadowld_key_id_query = 0x00;

	/* Set Shadowland's key id query */
	key_id_query = shadowld_key_id_query;

	/* Set Shadowland's key id */
	key_id = shadowld_key_id;

	/* set scrolling offsets for this game */
	{
		int bgx[4] = { 0x0b0, 0x0b2, 0x0b3, 0x0b4 };
		int bgy[4] = { 0x108, 0x108, 0x108, 0x008 };

		namcos1_set_scroll_offsets( bgx, bgy, 0 );
	}

	/* set sprite offsets for this game */
	namcos1_set_sprite_offsets( -120, 215 );

	/* build bank elements */
	namcos1_build_banks(rev1_key_r,rev1_key_w,0x03ff,0x03fb);
}

/**************************************************************************************
*	                                                                                  *
*	Splatter House specific                                                           *
*	                                                                                  *
**************************************************************************************/

/* Theres is an id check followed by some key nightmare */

void splatter_driver_init( void ) {

	/* set scrolling offsets for this game */
	{
		int bgx[4] = { 0x0b0, 0x0b2, 0x0b3, 0x0b4 };
		int bgy[4] = { 0x108, 0x108, 0x108, 0x008 };

		namcos1_set_scroll_offsets( bgx, bgy, 0 );
	}

	/* set sprite offsets for this game */
	namcos1_set_sprite_offsets( 0, 240 );

	/* build bank elements */
	namcos1_build_banks(rev2_key_r,rev2_key_w,0x03ff,0x037f);
}

/**************************************************************************************
*	                                                                                  *
*	World Stadium 90 specific                                                         *
*	                                                                                  *
**************************************************************************************/

/* Theres is an id check followed by some key nightmare */

void ws90_driver_init( void ) {

	key[0x47] = 0x36;
	key[0x40] = 0x36;

	/* set scrolling offsets for this game */
	{
		int bgx[4] = { 0x0b0, 0x0b2, 0x0b3, 0x0b4 };
		int bgy[4] = { 0x108, 0x108, 0x108, 0x008 };

		namcos1_set_scroll_offsets( bgx, bgy, 0 );
	}

	/* set sprite offsets for this game */
	namcos1_set_sprite_offsets( 0, 240-32 );

	/* build bank elements */
	namcos1_build_banks(rev1_key_r,rev1_key_w,0x03ff,0x03fb);
}
