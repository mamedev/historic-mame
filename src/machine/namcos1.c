#include "driver.h"
#include "vidhrdw/generic.h"

#define NAMCO_S1_RAM_REGION 5

/* from vidhrdw */
extern int namcos1_videoram_r( int offs );
extern void namcos1_videoram_w( int offs, int data );
extern void namcos1_set_scroll_offsets( int *bgx, int*bgy, int negative );
extern void namcos1_set_sprite_offsets( int x, int y );

static int (*key_r)( int offset );
static void (*key_w)( int offset, int data );

static unsigned char key[0x2000];

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
	int i;
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

/* RAM handlers */
static int rvk_r( int offset, int bank_offset ) {
	unsigned char *RAM = Machine->memory_region[NAMCO_S1_RAM_REGION];

	switch ( bank_offset ) {
		case 0x00: /* RAM 6 banks */
		case 0x01: /* palette goes here */
		case 0x02:
		case 0x03:
			return RAM[ ( bank_offset << 13 ) + offset ];
		break;

		case 0x08: /* RAM 5 banks - Videoram */
		case 0x09:
		case 0x0a:
		case 0x0b:
			bank_offset -= 0x08;
			return namcos1_videoram_r( ( bank_offset << 13 ) + offset );
		break;

		case 0x0c: /* Key chip bank */
			return (*key_r)( offset );
		break;

		case 0x0e: /* RAM 1 banks */
		case 0x0f:
			bank_offset -= 0x0e;
			RAM += 0x8000;
			return RAM[ ( bank_offset << 13 ) + offset ];
		break;

		default:
			if ( errorlog )
				fprintf( errorlog, "RVK_r handler: Accessing unknown sub bank %02x\n", bank_offset );
		break;
	}

	return 0;
}

static void rvk_w( int offset, int data, int bank_offset ) {
	unsigned char *RAM = Machine->memory_region[NAMCO_S1_RAM_REGION];

	switch ( bank_offset ) {
		case 0x00: /* RAM 6 banks - Palette - sprites */
		case 0x01: /* Palette - chars */
		case 0x02: /* chars? */
			RAM[ ( bank_offset << 13 ) + offset ] = data;

			if ( offset < 0x1800 ) {
				int color = ( offset & 0x7ff );
				int r = RAM[ ( bank_offset << 13 ) + color ];
				int g = RAM[ ( bank_offset << 13 ) + color + 0x800 ];
				int b = RAM[ ( bank_offset << 13 ) + color + 0x1000 ];

				palette_change_color( color + ( bank_offset * 0x800 ), r, g, b );
			}
		break;

		case 0x03: /* work ram */
			RAM[ ( bank_offset << 13 ) + offset ] = data;
		break;

		case 0x08: /* RAM 5 banks - Videoram */
		case 0x09:
		case 0x0a:
		case 0x0b:
			bank_offset -= 0x08;
			namcos1_videoram_w( ( bank_offset << 13 ) + offset, data );
		break;

		case 0x0c: /* Key chip bank */
			(*key_w)( offset, data );
		break;

		case 0x0e: /* RAM 1 banks */
		case 0x0f:
			bank_offset -= 0x0e;
			RAM += 0x8000;
			RAM[ ( bank_offset << 13 ) + offset ] = data;
		break;

		default:
			if ( errorlog )
				fprintf( errorlog, "RVK_w handler: Accessing unknown sub bank %02x\n", bank_offset );
		break;
	}
}

static int ram3_r( int offset, int bank_offset ) {
	unsigned char *RAM = Machine->memory_region[NAMCO_S1_RAM_REGION];

	switch ( bank_offset ) {
		case 0x00: /* RAM 3 banks */
		case 0x01:
		case 0x02:
		case 0x03:
			RAM += 0xc000;
			return RAM[ ( bank_offset << 13 ) + offset ];
		break;

		default:
			if ( errorlog )
				fprintf( errorlog, "RAM3_r handler: Accessing unknown sub bank %02x\n", bank_offset );
		break;
	}

	return 0;
}

static void ram3_w( int offset, int data, int bank_offset ) {
	unsigned char *RAM = Machine->memory_region[NAMCO_S1_RAM_REGION];

	switch ( bank_offset ) {
		case 0x00: /* RAM 3 banks */
		case 0x01:
		case 0x02:
		case 0x03:
			RAM += 0xc000;
			RAM[ ( bank_offset << 13 ) + offset ] = data;
		break;

		default:
			if ( errorlog )
				fprintf( errorlog, "RAM3_w handler: Accessing unknown sub bank %02x\n", bank_offset );
		break;
	}
}

/* ROM handlers */
#define PRG_HANDLER( num, offs ) static int prg##num##_r( int offset, int bank_offset ) { \
	unsigned char *RAM = Machine->memory_region[4] + offs; \
	return RAM[ ( bank_offset << 13 ) + offset ]; \
}

PRG_HANDLER( 0, 0xe0000 )
PRG_HANDLER( 1, 0xc0000 )
PRG_HANDLER( 2, 0xa0000 )
PRG_HANDLER( 3, 0x80000 )
PRG_HANDLER( 4, 0x60000 )
PRG_HANDLER( 5, 0x40000 )
PRG_HANDLER( 6, 0x20000 )
PRG_HANDLER( 7, 0x00000 )

#undef PRG_HANDLER

static void rom_w( int offset, int data, int bankoffset ) {
	if ( errorlog )
		fprintf(errorlog,"CPU #%d PC %04x: warning - write %02x to rom address %04x\n",cpu_getactivecpu(),cpu_getpc(),data,offset);
}

/* error handlers */
static int unknown_r( int offset, int bankoffset ) {
	if ( errorlog )
		fprintf(errorlog,"CPU #%d PC %04x: warning - read from unknown chip\n",cpu_getactivecpu(),cpu_getpc() );
	return 0;
}

static void unknown_w( int offset, int data, int bankoffset ) {
	if ( errorlog )
		fprintf(errorlog,"CPU #%d PC %04x: warning - wrote to unknown chip\n",cpu_getactivecpu(),cpu_getpc() );
}


/* Bank handler definitions */
typedef int (*handler_r)(int offset, int bank_offset);
typedef void (*handler_w)(int offset, int data, int bank_offset);

typedef struct {
	handler_r	bank_handler_r;
	handler_w	bank_handler_w;
	int			bank_offset;
} bankhandler;

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

		switch ( chip >> 4 ) {
			case 0x0017:	/* RAM - Video - Key */
				namcos1_banks[cpu][bank].bank_handler_r = rvk_r;
				namcos1_banks[cpu][bank].bank_handler_w = rvk_w;
				namcos1_banks[cpu][bank].bank_offset = chip & 0x0f;
			break;

			case 0x0018:	/* RAM3 */
				namcos1_banks[cpu][bank].bank_handler_r = ram3_r;
				namcos1_banks[cpu][bank].bank_handler_w = ram3_w;
				namcos1_banks[cpu][bank].bank_offset = chip & 0x0f;
			break;

			case 0x0020:	/* PRG0 */
			case 0x0021:	/* PRG0 */
			case 0x0022:	/* PRG0 */
			case 0x0023:	/* PRG0 */
				chip &= 0x0f;
				namcos1_banks[cpu][bank].bank_handler_r = prg0_r;
				namcos1_banks[cpu][bank].bank_handler_w = rom_w;
				namcos1_banks[cpu][bank].bank_offset = chip;
				/* Since we might be executing code, lets copy it to RAM */
				{
					unsigned char *src = ( Machine->memory_region[4] ) + 0xe0000;
					unsigned char *dst = Machine->memory_region[Machine->drv->cpu[cpu].memory_region];
					memcpy( &dst[bank << 13], &src[chip << 13], 0x2000 );
				}
			break;

			case 0x0024:	/* PRG1 */
			case 0x0025:	/* PRG1 */
			case 0x0026:	/* PRG1 */
			case 0x0027:	/* PRG1 */
				chip &= 0x0f;
				namcos1_banks[cpu][bank].bank_handler_r = prg1_r;
				namcos1_banks[cpu][bank].bank_handler_w = rom_w;
				namcos1_banks[cpu][bank].bank_offset = chip;
				/* Since we might be executing code, lets copy it to RAM */
				{
					unsigned char *src = ( Machine->memory_region[4] ) + 0xc0000;
					unsigned char *dst = Machine->memory_region[Machine->drv->cpu[cpu].memory_region];
					memcpy( &dst[bank << 13], &src[chip << 13], 0x2000 );
				}
			break;

			case 0x0028:	/* PRG2 */
			case 0x0029:	/* PRG2 */
			case 0x002a:	/* PRG2 */
			case 0x002b:	/* PRG2 */
				chip &= 0x0f;
				namcos1_banks[cpu][bank].bank_handler_r = prg2_r;
				namcos1_banks[cpu][bank].bank_handler_w = rom_w;
				namcos1_banks[cpu][bank].bank_offset = chip;
				/* Since we might be executing code, lets copy it to RAM */
				{
					unsigned char *src = ( Machine->memory_region[4] ) + 0xa0000;
					unsigned char *dst = Machine->memory_region[Machine->drv->cpu[cpu].memory_region];
					memcpy( &dst[bank << 13], &src[chip << 13], 0x2000 );
				}
			break;

			case 0x002c:	/* PRG3 */
			case 0x002d:	/* PRG3 */
			case 0x002e:	/* PRG3 */
			case 0x002f:	/* PRG3 */
				chip &= 0x0f;
				namcos1_banks[cpu][bank].bank_handler_r = prg3_r;
				namcos1_banks[cpu][bank].bank_handler_w = rom_w;
				namcos1_banks[cpu][bank].bank_offset = chip;
				/* Since we might be executing code, lets copy it to RAM */
				{
					unsigned char *src = ( Machine->memory_region[4] ) + 0x80000;
					unsigned char *dst = Machine->memory_region[Machine->drv->cpu[cpu].memory_region];
					memcpy( &dst[bank << 13], &src[chip << 13], 0x2000 );
				}
			break;

			case 0x0030:	/* PRG4 */
			case 0x0031:	/* PRG4 */
			case 0x0032:	/* PRG4 */
			case 0x0033:	/* PRG4 */
				chip &= 0x0f;
				namcos1_banks[cpu][bank].bank_handler_r = prg4_r;
				namcos1_banks[cpu][bank].bank_handler_w = rom_w;
				namcos1_banks[cpu][bank].bank_offset = chip;
				/* Since we might be executing code, lets copy it to RAM */
				{
					unsigned char *src = ( Machine->memory_region[4] ) + 0x60000;
					unsigned char *dst = Machine->memory_region[Machine->drv->cpu[cpu].memory_region];
					memcpy( &dst[bank << 13], &src[chip << 13], 0x2000 );
				}
			break;

			case 0x0034:	/* PRG5 */
			case 0x0035:	/* PRG5 */
			case 0x0036:	/* PRG5 */
			case 0x0037:	/* PRG5 */
				chip &= 0x0f;
				namcos1_banks[cpu][bank].bank_handler_r = prg5_r;
				namcos1_banks[cpu][bank].bank_handler_w = rom_w;
				namcos1_banks[cpu][bank].bank_offset = chip;
				/* Since we might be executing code, lets copy it to RAM */
				{
					unsigned char *src = ( Machine->memory_region[4] ) + 0x40000;
					unsigned char *dst = Machine->memory_region[Machine->drv->cpu[cpu].memory_region];
					memcpy( &dst[bank << 13], &src[chip << 13], 0x2000 );
				}
			break;

			case 0x0038:	/* PRG6 */
			case 0x0039:	/* PRG6 */
			case 0x003a:	/* PRG6 */
			case 0x003b:	/* PRG6 */
				chip &= 0x0f;
				namcos1_banks[cpu][bank].bank_handler_r = prg6_r;
				namcos1_banks[cpu][bank].bank_handler_w = rom_w;
				namcos1_banks[cpu][bank].bank_offset = chip;
				/* Since we might be executing code, lets copy it to RAM */
				{
					unsigned char *src = ( Machine->memory_region[4] ) + 0x20000;
					unsigned char *dst = Machine->memory_region[Machine->drv->cpu[cpu].memory_region];
					memcpy( &dst[bank << 13], &src[chip << 13], 0x2000 );
				}
			break;

			case 0x003c:	/* PRG7 */
			case 0x003d:	/* PRG7 */
			case 0x003e:	/* PRG7 */
			case 0x003f:	/* PRG7 */
				chip &= 0x0f;
				namcos1_banks[cpu][bank].bank_handler_r = prg7_r;
				namcos1_banks[cpu][bank].bank_handler_w = rom_w;
				namcos1_banks[cpu][bank].bank_offset = chip;
				/* Since we might be executing code, lets copy it to RAM */
				{
					unsigned char *src = Machine->memory_region[4];
					unsigned char *dst = Machine->memory_region[Machine->drv->cpu[cpu].memory_region];
					memcpy( &dst[bank << 13], &src[chip << 13], 0x2000 );
				}
			break;

			default:
				if ( errorlog )
					fprintf( errorlog, "CPU %d : Unknown chip selected ($%04x) at PC %04x\n", cpu_getactivecpu(), chip, cpu_getpc() );

				namcos1_banks[cpu][bank].bank_handler_r = unknown_r;
				namcos1_banks[cpu][bank].bank_handler_w = unknown_w;
				namcos1_banks[cpu][bank].bank_offset = chip;
			break;
		}
	} else {
		chip &= 0x00ff;
		chip |= ( data & 0xff ) << 8;
	}
}

int namcos1_banked_area_r( int offset ) {

	int bank = ( offset >> 13 ) & 0x07;
	int cpu = cpu_getactivecpu();

	return (*namcos1_banks[cpu][bank].bank_handler_r)( offset & 0x1fff, namcos1_banks[cpu][bank].bank_offset );
}

void namcos1_banked_area_w( int offset, int data ) {

	int bank = ( offset >> 13 ) & 0x07;
	int cpu = cpu_getactivecpu();

	(*namcos1_banks[cpu][bank].bank_handler_w)( offset & 0x1fff, data, namcos1_banks[cpu][bank].bank_offset );
}

/**************************************************************************************
*	                                                                                  *
*	63701 MCU emulation (CUS64)                                                       *
*	                                                                                  *
**************************************************************************************/

static int mcu_first_time;

void namcos1_mcu_control_w( int offset, int data ) {

	if ( data & 1 ) {
		/* The first MCU check is done on the 'kernel' code section we're missing */
		/* We just skip it */

		if ( mcu_first_time ) {
			mcu_first_time = 0;
			cpu_halt( 3, 0 );
			rvk_w( 0x1000, 0x00, 0x0f );
		} else
			cpu_halt( 3, 1 );

		cpu_reset( 3 );
		cpu_halt( 1, 1 );
	} else
		cpu_halt( 1, 0 );
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

	/* We never really change the OP base, but this will stop MAME from reporting */
	/* execution on mapped io when running code in our banks                      */
	return -1;
}

/* default values for cpu start banks */
static int cpu0_start = 0x03ff;
static int cpu1_start = 0x03fb;

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

	/* Fake MCU reset vector & regenerate mcu functionality */
	/* This MIGHT be Pacmania specific. I'll look into it later */
	{
		unsigned char	*RAM = Machine->memory_region[7];
		unsigned int	my_pc;

		memset( &RAM[0xf000], 0, 0x1000 );

		/* Reset vector */
		RAM[0xfffe] = 0xb8;
		RAM[0xffff] = 0x00;
#define ASSEMBLE_START(a) my_pc = a
#define ASSEMBLE(a) RAM[my_pc++] = a

		ASSEMBLE_START(0xf469);

		/* Skip callbacks if idling */
		ASSEMBLE(0xb6); ASSEMBLE(0xc0); ASSEMBLE(0x2f); /* lda	$c02f */
		ASSEMBLE(0x26); ASSEMBLE(0x05); /* bne +5 */
		ASSEMBLE(0x38); /* pulx */
		ASSEMBLE(0x08); /* inx */
		ASSEMBLE(0x08); /* inx */
		ASSEMBLE(0x08); /* inx */
		ASSEMBLE(0x3c); /* pshx */

		ASSEMBLE(0x39); /* rts */

		/* IRQ vector */
		RAM[0xfff8] = ( my_pc >> 8 ) & 0xff;
		RAM[0xfff9] = ( my_pc & 0xff );

		/* Signal we're working */
		ASSEMBLE(0x86); ASSEMBLE(0xa6); /* lda #$a6 */
		ASSEMBLE(0xb7); ASSEMBLE(0xc0); ASSEMBLE(0x00); /* sta	$c000 */

		/* Process voice commands too */
		ASSEMBLE(0x86); ASSEMBLE(0x01); /* lda #$01 */
		ASSEMBLE(0x98); ASSEMBLE(0xb9); /* eora $b9 */
		ASSEMBLE(0x97); ASSEMBLE(0xb9); /* sta $b9 */

		ASSEMBLE(0x3b); /* rti */

		/* This extra byte makes sure the rom checksum match */
		ASSEMBLE(0x46);

#undef ASSEMBLE_START
#undef ASSEMBLE
	}

	/* Point mcu & sound shared RAM to destination */
	{
		unsigned char *RAM = ( Machine->memory_region[NAMCO_S1_RAM_REGION] ) + 0xb000; /* Ram 1, bank 1, offset 0x1000 */
		cpu_setbank( 3, RAM );
		cpu_setbank( 4, RAM );
	}

	/* Init MCU checks */
	mcu_first_time = 1;

	/* In case we had some cpu's suspended, resume them now */
	cpu_halt( 1, 1 );
	cpu_halt( 3, 1 );

	cpu_setOPbaseoverride( namcos1_setopbase );
}

/**************************************************************************************
*	                                                                                  *
*	Blazer specific			                                                          *
*	                                                                                  *
**************************************************************************************/

/* Theres is an id check followed by some key nightmare */

void blazer_driver_init( void ) {

	key_r = rev2_key_r;
	key_w = rev2_key_w;

	key_id = 0x13;

	/* set scrolling offsets for this game */
	{
		int bgx[4] = { 0x1d0, 0x1d2, 0x1d3, 0x1d4 };
		int bgy[4] = { 0x1e8, 0x1e8, 0x1e8, 0x0e8 };

		namcos1_set_scroll_offsets( bgx, bgy, 1 );
	}

	/* set sprite offsets for this game */
	namcos1_set_sprite_offsets( -120, 232 );
}

/**************************************************************************************
*	                                                                                  *
*	Dragon Spirits specific                                                           *
*	                                                                                  *
**************************************************************************************/

/* Theres is an id check followed by some key nightmare */

void dspirits_driver_init( void ) {

	key_r = rev2_key_r;
	key_w = rev2_key_w;

	key_id = 0x36;

	/* set scrolling offsets for this game */
	{
		int bgx[4] = { 0x1d0, 0x1d2, 0x1d3, 0x1d4 };
		int bgy[4] = { 0x1e8, 0x1e8, 0x1e8, 0x0e8 };

		namcos1_set_scroll_offsets( bgx, bgy, 1 );
	}

	/* set sprite offsets for this game */
	namcos1_set_sprite_offsets( -120, 232 );
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

	key_r = rev1_key_r;
	key_w = rev1_key_w;

	/* set scrolling offsets for this game */
	{
		int bgx[4] = { 0x1d0, 0x1d2, 0x1d3, 0x1d4 };
		int bgy[4] = { 0x1e8, 0x1e8, 0x1e8, 0x0e8 };

		namcos1_set_scroll_offsets( bgx, bgy, 1 );
	}

	/* set sprite offsets for this game */
	namcos1_set_sprite_offsets( -120, 232 );
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

	key_r = rev1_key_r;
	key_w = rev1_key_w;

	/* set scrolling offsets for this game */
	{
		int bgx[4] = { 0x0b0, 0x0b2, 0x0b3, 0x0b4 };
		int bgy[4] = { 0x108, 0x108, 0x108, 0x008 };

		namcos1_set_scroll_offsets( bgx, bgy, 0 );
	}

	/* set sprite offsets for this game */
	namcos1_set_sprite_offsets( -56, 240 );
}

/**************************************************************************************
*	                                                                                  *
*	Splatter House specific                                                           *
*	                                                                                  *
**************************************************************************************/

/* Theres is an id check followed by some key nightmare */

void splatter_driver_init( void ) {

	key_r = rev2_key_r;
	key_w = rev2_key_w;

	/* cpu1 start bank is different */
	/* prg5, last bank */
	cpu1_start = 0x037f;

	/* set scrolling offsets for this game */
	{
		int bgx[4] = { 0x0b0, 0x0b2, 0x0b3, 0x0b4 };
		int bgy[4] = { 0x108, 0x108, 0x108, 0x008 };

		namcos1_set_scroll_offsets( bgx, bgy, 0 );
	}

	/* set sprite offsets for this game */
	namcos1_set_sprite_offsets( -56, 240 );
}

/**************************************************************************************
*	                                                                                  *
*	World Stadium 90 specific                                                         *
*	                                                                                  *
**************************************************************************************/

/* Theres is an id check followed by some key nightmare */

void ws90_driver_init( void ) {

	key_r = rev1_key_r;
	key_w = rev1_key_w;

	/* set scrolling offsets for this game */
	{
		int bgx[4] = { 0x0b0, 0x0b2, 0x0b3, 0x0b4 };
		int bgy[4] = { 0x108, 0x108, 0x108, 0x008 };

		namcos1_set_scroll_offsets( bgx, bgy, 0 );
	}

	/* set sprite offsets for this game */
	namcos1_set_sprite_offsets( -56, 240 );
}
