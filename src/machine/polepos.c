#include "driver.h"
#include "cpu/z80/z80.h"

/*************************************************************************************/
/* 4 bit cpu emulation                                                               */
/*************************************************************************************/

static struct polepos_mcu_def {
	int enabled;		/* Enabled */
	int status;			/* Status */
	int transfer_id;	/* Transfer id */
	void *timer;		/* Transfer timer */
} polepos_mcu;

void polepos_mcu_enable_w( int offs, int data ) {
	polepos_mcu.enabled = data & 1;
	
	if ( polepos_mcu.enabled == 0 ) { /* If its getting disabled, kill our timer */
		if ( polepos_mcu.timer ) {
			timer_remove( polepos_mcu.timer );
			polepos_mcu.timer = 0;
		}
	}
}

static void polepos_mcu_callback( int param ) {
	cpu_cause_interrupt( 0, Z80_NMI_INT );
}

int polepos_mcu_control_r( int offs ) {
	if ( polepos_mcu.enabled )
		return polepos_mcu.status;
	
	return 0x00;
}

void polepos_mcu_control_w( int offs, int data ) {
	if ( errorlog )
		fprintf( errorlog, "polepos_mcu_control_w: %d, $%02x\n", offs, data );
    if ( polepos_mcu.enabled ) {
		if ( data != 0x10 ) { /* start transfer */
			polepos_mcu.transfer_id = data; /* get the id */
			polepos_mcu.status = 0xe0; /* set status */
			if ( polepos_mcu.timer )
				timer_remove( polepos_mcu.timer );
			/* fire off the transfer timer */
			polepos_mcu.timer = timer_pulse( TIME_IN_USEC( 50 ), 0, polepos_mcu_callback );
		} else { /* end transfer */
			if ( polepos_mcu.timer ) /* shut down our transfer timer */
				timer_remove( polepos_mcu.timer );
			polepos_mcu.timer = 0;
			polepos_mcu.status = 0x10; /* set status */
		}
	}
}

int polepos_mcu_data_r( int offs ) {
	if ( polepos_mcu.enabled ) {
		switch( polepos_mcu.transfer_id ) {
			case 0x71: /* 3 bytes */
				switch ( offs ) {
					case 0x00:
						return ~readinputport( 0 ); /* Service, Gear, etc */
					break;
					
					case 0x01:
						return ~readinputport( 2 ); /* DSW1 */
					break;
				}				
			break;
			
			case 0x72: /* 8 bytes */
				switch ( offs ) {
					case 0x04:
						return ~readinputport( 1 ); /* DSW 0 */
					break;
				}
			break;
			
			case 0x91: /* 3 bytes */
				switch ( offs ) {
					case 0x00:
						return 0x00;
					break;
				}
			break;
			
			case 0xa1: /* 8 bytes */
			break;
			
			default:
				if ( errorlog )
					fprintf( errorlog, "Unknwon MCU transfer mode: %02x\n", polepos_mcu.transfer_id );
			break;
		}
	}
	
	return 0xff; /* pull up */
}

void polepos_mcu_data_w( int offs, int data ) {
}

void polepos_init_machine( void ) {
	/* Initialize the MCU */
	polepos_mcu.enabled = 0; /* disabled */
	polepos_mcu.status = 0x10; /* ready to transfer */
	polepos_mcu.transfer_id = 0; /* clear out the transfer id */
	if ( polepos_mcu.timer ) /* disable our timer */
		timer_remove( polepos_mcu.timer );
	polepos_mcu.timer = 0;
}
