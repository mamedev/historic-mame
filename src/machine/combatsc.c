/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "cpu/m6809/m6809.h"

extern void combatsc_io_w( int offset, int data );
extern int combatsc_io_r( int offset );
extern unsigned char *combatsc_io_ram;

int combatsc_bank_select; /* 0x00..0x1f */
int combatsc_video_circuit; /* 0 or 1 */
unsigned char *combatsc_page0;
unsigned char *combatsc_page1;
unsigned char *combatsc_workram0;
unsigned char *combatsc_workram1;
unsigned char *combatsc_workram;

extern unsigned char *videoram;


static int banks[0x20] = {
	0x00000, 0x04000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0x10000, 0, 0x14000, 0, 0x18000, 0, 0x1c000, 0,
	0x20000, 0, 0x24000, 0, 0x28000, 0, 0x2c000, 0x04000 };

void combatsc_bankselect_w( int offset, int data ){
	if( data & 0x40 ){
		combatsc_video_circuit = 1;
		videoram = combatsc_page1;
		combatsc_workram = combatsc_workram1;
	}
	else {
		combatsc_video_circuit = 0;
		videoram = combatsc_page0;
		combatsc_workram = combatsc_workram0;
	}

	data = data & 0x1f;
	if( data != combatsc_bank_select ){
		unsigned char *page = &Machine->memory_region[0][0x10000];
		combatsc_bank_select = data;

		cpu_setbank (1, page + banks[data]);

		if (data == 0x1f){
			cpu_setbankhandler_r (1, combatsc_io_r);/* IO RAM & Video Registers */
			cpu_setbankhandler_w (1, combatsc_io_w);
		}
		else{
			cpu_setbankhandler_r (1, MRA_BANK1);	/* banked ROM */
			cpu_setbankhandler_w (1, MWA_ROM);
		}
	}
}

void combatsc_init_machine( void )
{
	unsigned char *MEM = Machine->memory_region[0];

	/* Set optimization flags for M6809 */
	m6809_Flags = M6809_FAST_NONE;

	memcpy( &MEM[0x8000], &MEM[0x18000], 0x8000 ); /* map upper half of ROM */

	combatsc_workram0 = &MEM[0x40000];
	combatsc_workram1 = &MEM[0x40060];
	combatsc_io_ram	= &MEM[0x40000];
	combatsc_page0	= &MEM[0x44000];
	combatsc_page1	= &MEM[0x46000];

	memset( combatsc_io_ram,	0x00, 0x4000 );
	memset( combatsc_page0,		0x00, 0x2000 );
	memset( combatsc_page1,		0x00, 0x2000 );

	combatsc_bank_select = -1;
	combatsc_bankselect_w( 0,0 );
}

int combatsc_workram_r( int offset ){
	return combatsc_workram[offset];
}

void combatsc_workram_w( int offset, int data ){
	combatsc_workram[offset] = data;
}

void combatsc_coin_counter_w(int offset,int data){
	/* b7-b3: unused? */
	/* b1: coin counter 2 */
	/* b0: coin counter 1 */

	coin_counter_w(0,(data) & 0x01);
	coin_counter_w(1,(data >> 1) & 0x01);
}

