
#include "driver.h"
#include "machine/8255ppi.h"

static ppi8255_interface	*intf; /* local copy of the intf */

typedef struct {
	int		groupA_mode;
	int		groupB_mode;
	int		io[3];
	int		latch[3];
	int		control;
} ppi8255;

static ppi8255 chips[MAX_8255];

void ppi8255_init( ppi8255_interface *intfce ) {
	int i;

	intf = intfce; /* keep a local pointer to the interface */

	for ( i = 0; i < intf->num; i++ ) {
		chips[i].groupA_mode = 0; /* group a mode */
		chips[i].groupB_mode = 0; /* group b mode */
		chips[i].io[0] = 0xff; /* all inputs */
		chips[i].io[1] = 0xff; /* all inputs */
		chips[i].io[2] = 0xff; /* all inputs */
		chips[i].latch[0] = 0x00; /* clear latch */
		chips[i].latch[1] = 0x00; /* clear latch */
		chips[i].latch[2] = 0x00; /* clear latch */
		chips[i].control = 0x1b;
	}
}

int ppi8255_r( int which, int offset ) {
	ppi8255		*chip;

	/* Some bounds checking */
	if ( which > intf->num ) {
		if ( errorlog )
			fprintf( errorlog, "Attempting to read an unmapped 8255 chip\n" );
		return 0;
	}

	if ( offset > 3 ) {
		if ( errorlog )
			fprintf( errorlog, "Attempting to read an invalid 8255 register\n" );
		return 0;
	}

	chip = &chips[which];

	switch( offset ) {
		case 0: /* Port A read */
			if ( chip->io[0] == 0 )
				return chip->latch[0];
			else {
				if ( intf->portA_r )
					return (*intf->portA_r)( which );
			}
		break;

		case 1: /* Port B read */
			if ( chip->io[1] == 0 )
				return chip->latch[1];
			else {
				if ( intf->portB_r )
					return (*intf->portB_r)( which );
			}
		break;

		case 2: /* Port C read */
			if ( chip->io[2] == 0 )
				return chip->latch[2];
			else {
				int input = 0;
				if ( intf->portC_r )
					input = (*intf->portC_r)( which );

				input &= chip->io[2];
				input |= chip->latch[2] & ~chip->io[2];

				return input;
			}
		break;

		case 3: /* Control word */
			return chip->control;
		break;
	}

	if ( errorlog )
		fprintf( errorlog, "8255 chip %d: Port %c is being read but has no handler", which, 'A' + offset );

	return 0x00;
}

void ppi8255_w( int which, int offset, int data ) {
	ppi8255		*chip;

	/* Some bounds checking */
	if ( which > intf->num ) {
		if ( errorlog )
			fprintf( errorlog, "Attempting to write an unmapped 8255 chip\n" );
		return;
	}

	if ( offset > 3 ) {
		if ( errorlog )
			fprintf( errorlog, "Attempting to write an invalid 8255 register\n" );
		return;
	}

	chip = &chips[which];

	switch( offset ) {
		case 0: /* Port A write */
			if ( chip->io[0] == 0 ) {
				chip->latch[0] = data;
				if ( intf->portA_w )
					(*intf->portA_w)( which, data );
			}
			return;
		break;

		case 1: /* Port B write */
			if ( chip->io[1] == 0 ) {
				chip->latch[1] = data;
				if ( intf->portB_w )
					(*intf->portB_w)( which, data );
			}
			return;
		break;

		case 2: /* Port C write */
			if ( chip->io[2] != 0xff ) {
				if ( chip->io[2] != 0 ) {
					chip->latch[2] = ( chip->latch[2] & chip->io[2] ) | ( data & ~chip->io[2] );
					if ( intf->portC_w )
						(*intf->portC_w)( which, data & ~chip->io[2] );
				} else {
					chip->latch[2] = data;
					if ( intf->portC_w )
						(*intf->portC_w)( which, data );
				}
			}
			return;
		break;

		case 3: /* Control word */
			chip->control = data;

			if ( data & 0x80 ) { /* mode set */
				chip->groupA_mode = ( data >> 5 ) & 3;
				chip->groupB_mode = ( data >> 2 ) & 1;

				if ( chip->groupA_mode != 0 || chip->groupB_mode != 0 ) {
					if ( errorlog )
						fprintf( errorlog, "8255 chip %d: Setting an unsupported mode!\n", which );
				}
			}

			/* Port A direction */
			if ( data & 0x10 )
				chip->io[0] = 0xff;
			else
				chip->io[0] = 0x00;

			/* Port B direction */
			if ( data & 0x02 )
				chip->io[1] = 0xff;
			else
				chip->io[1] = 0x00;

			/* Port C upper direction */
			if ( data & 0x08 )
				chip->io[2] |= 0xf0;
			else
				chip->io[2] &= 0x0f;

			/* Port C lower direction */
			if ( data & 0x01 )
				chip->io[2] |= 0x0f;
			else
				chip->io[2] &= 0xf0;

			return;
		break;
	}

	if ( errorlog )
		fprintf( errorlog, "8255 chip %d: Port %c is being written to but has no handler", which, 'A' + offset );
}

/* Helpers */
int ppi8255_0_r( int offset ) { return ppi8255_r( 0, offset ); }
int ppi8255_1_r( int offset ) { return ppi8255_r( 1, offset ); }
int ppi8255_2_r( int offset ) { return ppi8255_r( 2, offset ); }
int ppi8255_3_r( int offset ) { return ppi8255_r( 3, offset ); }
void ppi8255_0_w( int offset, int data ) { ppi8255_w( 0, offset, data ); }
void ppi8255_1_w( int offset, int data ) { ppi8255_w( 1, offset, data ); }
void ppi8255_2_w( int offset, int data ) { ppi8255_w( 2, offset, data ); }
void ppi8255_3_w( int offset, int data ) { ppi8255_w( 3, offset, data ); }
