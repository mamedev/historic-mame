#include "driver.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/konamiic.h"
#include "cpu/konami/konami.h"

/* from vidhrdw */
extern void simpsons_video_banking( int select );

int simpsons_firq_enabled;

/***************************************************************************

  EEPROM

***************************************************************************/

static int eeprom_ack;
static int eeprom_data;
static int eeprom_mode;
static unsigned char eeprom_memory[128];
static int eeprom_unlocked;

int konami_eeprom_read( void ) {
	int data = ( eeprom_data >> 7 ) & 1; /* get the bit to read */

	eeprom_data <<= 1; /* update the shift read register */

	return data;
}

int konami_eeprom_ack( void ) {
	int data = eeprom_ack;

	eeprom_ack = 0; /* clear on read */

	return data;
}

int simpsons_eeprom_r( int offset )
{
	int data = konami_eeprom_read();

	data <<= 4; /* put it on the right pin */

	data |= konami_eeprom_ack() << 5; /* add the ack */

	data |= readinputport( 5 ) & 1; /* test switch */

	return data;
}

void konami_eeprom_w( int clk, int dat, int active )
{
	static int last_clk = 0, bit_count = 0;
	static int shift_register = 0, write_pending;
	static int eeprom_addr;

	/* latch on 0 -> 1 transition */
	if ( last_clk == 0 && clk == 1 )
	{
		if ( active == 1 )
		{ /* if active */
			shift_register <<= 1;
			shift_register |= dat;
			bit_count++;
			if ( bit_count == 13 )
			{
				switch ( shift_register & 0x1f80 )
				{
					case 0x980: /* unlock command */
						eeprom_unlocked = 1;
					break;

					case 0xe00: /* write data */
						eeprom_addr = shift_register & 0x7f; /* fetch address */
						if ( eeprom_unlocked )
							write_pending = 1;
					break;

					case 0xc00: /* read data */
						eeprom_addr = shift_register & 0x7f; /* fetch address */
						eeprom_data = eeprom_memory[eeprom_addr]; /* fetch data */
					break;

					case 0x800: /* lock command */
						eeprom_unlocked = 0;
					break;
				}
			}

			if ( bit_count > 31 )
			{
				bit_count = 0;
				shift_register = 0;
				if ( errorlog )
					fprintf( errorlog, "Eeprom serial shift register overflow\n" );
			}
		}
		else
		{
			/* on to off transition */
			if ( eeprom_mode == 1 )
			{
				/* see if we have a write pending */
				if ( write_pending )
				{
					eeprom_memory[eeprom_addr] = shift_register & 0xff; /* do the write */
					eeprom_ack = 1; /* signal we're succesfull */
					write_pending = 0;
				}
				bit_count = 0;
				shift_register = 0;
			}
		}

		eeprom_mode = active;
	}

	last_clk = clk;
}

void simpsons_eeprom_w( int offset, int data )
{
	int clk = ( data >> 4 ) & 1;
	int dat = ( data >> 7 ) & 1; /* data pin */
	int mode = ( data >> 3 ) & 1; /* 1 = selected - 0 = deselected */

	if ( data == 0xff )
		return;

	konami_eeprom_w( clk, dat, mode );

	simpsons_video_banking( data & 3 );

	simpsons_firq_enabled = ( ( data >> 2 ) & 1 ) && ( ( data & 3 ) == 0 );
}

/***************************************************************************

  Coin Counters, Sound Interface

***************************************************************************/

void simpsons_coin_counter_w( int offset, int data )
{
	/* bit 0,1 coin counters */
	coin_counter_w(0,data & 0x01);
	coin_counter_w(1,data & 0x02);
	/* bit 2 selects mono or stereo sound */
	/* bit 3 = enable char ROM reading through the video RAM */
	K052109_set_RMRD_line( ( data & 0x08 ) ? ASSERT_LINE : CLEAR_LINE );
	/* bit 4 = INIT (unknown) */
	/* bit 5 = OBJCHA (unknown, sprite related) */
}

int simpsons_sound_interrupt_r( int offset )
{
	cpu_cause_interrupt( 1, 0xff );
	return 0x00;
}

int simpsons_sound_r(int offset)
{
	/* If the sound CPU is running, read the status, otherwise
	   just make it pass the test */
	if (Machine->sample_rate != 0) 	return K053260_ReadReg(2 + offset);
	else
	{
		static int res = 0x80;

		res = (res & 0xfc) | ((res + 1) & 0x03);
		return offset ? res : 0x00;
	}
}

/***************************************************************************

  Speed up memory handlers

***************************************************************************/

int simpsons_speedup1_r( int offs )
{
	unsigned char *RAM = Machine->memory_region[0];

	int data1 = RAM[0x486a];

	if ( data1 == 0 )
	{
		int data2 = ( RAM[0x4942] << 8 ) | RAM[0x4943];

		if ( data2 < Machine->memory_region_length[0] )
		{
			data2 = ( RAM[data2] << 8 ) | RAM[data2 + 1];

			if ( data2 == 0xffff )
				cpu_spinuntil_int();

			return RAM[0x4942];
		}

		return RAM[0x4942];
	}

	if ( data1 == 1 )
		RAM[0x486a]--;

	return RAM[0x4942];
}

int simpsons_speedup2_r( int offs )
{
	int data = Machine->memory_region[0][0x4856];

	if ( data == 1 )
		cpu_spinuntil_int();

	return data;
}

/***************************************************************************

  Banking, initialization

***************************************************************************/

static void simpsons_banking( int lines )
{
	unsigned char *RAM = Machine->memory_region[0];
	int offs = 0;

	switch ( lines & 0xf0 )
	{
		case 0x00: /* simp_g02.rom */
			offs = 0x10000 + ( ( lines & 0x0f ) * 0x2000 );
		break;

		case 0x10: /* simp_p01.rom */
			offs = 0x30000 + ( ( lines & 0x0f ) * 0x2000 );
		break;

		case 0x20: /* simp_013.rom */
			offs = 0x50000 + ( ( lines & 0x0f ) * 0x2000 );
		break;

		case 0x30: /* simp_012.rom ( lines goes from 0x00 to 0x0c ) */
			offs = 0x70000 + ( ( lines & 0x0f ) * 0x2000 );
		break;

		default:
			if ( errorlog )
				fprintf( errorlog, "PC = %04x : Unknown bank selected (%02x)\n", cpu_get_pc(), lines );
		break;
	}

	cpu_setbank( 1, &RAM[offs] );
}

void simpsons_init_machine( void )
{
	unsigned char *RAM = Machine->memory_region[0];

	konami_cpu_setlines_callback = simpsons_banking;

	paletteram = &RAM[0x88000];
	spriteram = &RAM[0x8e000];
	simpsons_firq_enabled = 0;

	/* init the default banks */
	cpu_setbank( 1, &RAM[0x10000] );

	RAM = Machine->memory_region[3];

	cpu_setbank( 2, &RAM[0x10000] );

	simpsons_video_banking( 0 );
}

int simpsons_eeprom_load(void)
{
	void *f;


	/* Try loading static RAM */
	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
	{
		/* just load in everything, the game will overwrite what it doesn't want */
		osd_fread(f,eeprom_memory,128);
		osd_fclose(f);
	}
	else
	{
		memset(eeprom_memory,0xff,128);
		usrintf_showmessage("Press F2+F3 to reset EEPROM");
	}

	return 1;
}

void simpsons_eeprom_save(void)
{
	void *f;


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,eeprom_memory,128);
		osd_fclose(f);
	}
}
