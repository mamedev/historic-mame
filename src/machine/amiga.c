/***************************************************************************
Amiga Computer / Arcadia Game System

Driver by:

Ernesto Corvi & Mariusz Wojcieszek

***************************************************************************/

#include "driver.h"
#include "includes/amiga.h"
#include "cpu/m68000/m68000.h"

#define LOG_CUSTOM	1
#define LOG_CIA		0

static const struct amiga_machine_interface *amiga_intf;



/***************************************************************************

    General routines and registers

***************************************************************************/

/* required prototype */
WRITE16_HANDLER(amiga_custom_w);

custom_regs_def custom_regs;

UINT16 *amiga_chip_ram;
UINT16 *amiga_expansion_ram;
UINT16 *amiga_autoconfig_mem;

static int translate_ints( void ) {

	int ints = CUSTOM_REG(REG_INTENA) & CUSTOM_REG(REG_INTREQ);
	int ret = 0;

	if ( CUSTOM_REG(REG_INTENA) & 0x4000 ) { /* Master interrupt switch */

		/* Level 7 = NMI Can only be triggered from outside the hardware */

		if ( ints & 0x2000 )
			ret |= ( 1 << 5 );

		if ( ints & 0x1800 )
			ret |= ( 1 << 4 );

		if ( ints & 0x0780 )
			ret |= ( 1 << 3 );

		if ( ints & 0x0070 )
			ret |= ( 1 << 2 );

		if ( ints & 0x0008 )
			ret |= ( 1 << 1 );

		if ( ints & 0x0007 )
			ret |= ( 1 << 0 );
	}

	return ret;
}

static void check_ints( void ) {

	int ints = translate_ints(), i;

	for ( i = 0; i < 7; i++ ) {
		if ( ( ints >> i ) & 1 )
			cpunum_set_input_line( 0, i + 1, ASSERT_LINE );
		else
			cpunum_set_input_line( 0, i + 1, CLEAR_LINE );
	}
}


/***************************************************************************

    Blitter emulation

***************************************************************************/

static unsigned short blitter_fill( unsigned short src, int mode, int *fc ) {
	int i, data;
	unsigned short dst = 0;

	for ( i = 0; i < 16; i++ ) {
		data = ( src >> i ) & 1;
		if ( data ) {
			*fc ^= 1;
			if ( mode & 0x0010 ) /* Exclusive mode */
				dst |= ( data ^ *fc ) << i;
			if ( mode & 0x0008 ) /* Inclusive mode */
				dst |= ( data | *fc ) << i;
		} else
			dst |= ( *fc << i );
	}

	return dst;
}

/* Ascending */
#define BLIT_FUNCA( num, op ) static int blit_func_a##num ( unsigned short *old_data,		\
															unsigned short *new_data,		\
														 	int first, int last ) {			\
	unsigned short dataA, dataB, dataC; 													\
	int shiftA, shiftB;																		\
																							\
	dataA =	new_data[0];																	\
	dataB =	new_data[1];																	\
	dataC =	new_data[2];																	\
																							\
	if ( first )																			\
		dataA &= CUSTOM_REG(REG_BLTAFWM);													\
																							\
	if ( last )																				\
		dataA &= CUSTOM_REG(REG_BLTALWM);													\
																							\
	shiftA = ( CUSTOM_REG(REG_BLTCON0) >> 12 ) & 0x0f;										\
	shiftB = ( CUSTOM_REG(REG_BLTCON1) >> 12 ) & 0x0f;										\
																							\
	if ( shiftA ) {																			\
		dataA >>= shiftA;																	\
		if ( !first )																		\
			dataA |= ( old_data[0] & ( ( 1 << shiftA ) - 1 ) ) << ( 16 - shiftA );			\
	}																						\
																							\
	if ( shiftB ) {																			\
		dataB >>= shiftB;																	\
		if ( !first )																		\
			dataB |= ( old_data[1] & ( ( 1 << shiftB ) - 1 ) ) << ( 16 - shiftB );			\
	}																						\
																							\
	return ( op );																			\
}

BLIT_FUNCA( 0, ( ~dataA & ~dataB & ~dataC ) ) /* /A/B/C */
BLIT_FUNCA( 1, ( ~dataA & ~dataB &  dataC ) ) /* /A/B C */
BLIT_FUNCA( 2, ( ~dataA &  dataB & ~dataC ) ) /* /A B/C */
BLIT_FUNCA( 3, ( ~dataA &  dataB &  dataC ) ) /* /A B C */
BLIT_FUNCA( 4, (  dataA & ~dataB & ~dataC ) ) /*  A/B/C */
BLIT_FUNCA( 5, (  dataA & ~dataB &  dataC ) ) /*  A/B C */
BLIT_FUNCA( 6, (  dataA &  dataB & ~dataC ) ) /*  A B/C */
BLIT_FUNCA( 7, (  dataA &  dataB &  dataC ) ) /*  A B C */

static int (*blit_func_a[8])( unsigned short *old_data, unsigned short *new_data, int first, int last ) = {
	blit_func_a0,
	blit_func_a1,
	blit_func_a2,
	blit_func_a3,
	blit_func_a4,
	blit_func_a5,
	blit_func_a6,
	blit_func_a7
};

/* Descending */
#define BLIT_FUNCD( num, op ) static int blit_func_d##num ( unsigned short *old_data,		\
															unsigned short *new_data,		\
															int first, int last ) {			\
	unsigned short dataA, dataB, dataC; 													\
	int shiftA, shiftB;																		\
																							\
	dataA =	new_data[0];																	\
	dataB =	new_data[1];																	\
	dataC =	new_data[2];																	\
																							\
	if ( first )																			\
		dataA &= CUSTOM_REG(REG_BLTAFWM);													\
																							\
	if ( last )																				\
		dataA &= CUSTOM_REG(REG_BLTALWM);													\
																							\
	shiftA = ( CUSTOM_REG(REG_BLTCON0) >> 12 ) & 0x0f;										\
	shiftB = ( CUSTOM_REG(REG_BLTCON1) >> 12 ) & 0x0f;										\
																							\
	if ( shiftA ) {																			\
		dataA <<= shiftA;																	\
		if ( !first )																		\
			dataA |= ( old_data[0] >> ( 16 - shiftA ) ) & ( ( 1 << shiftA ) - 1 );			\
	}																						\
																							\
	if ( shiftB ) {																			\
		dataB <<= shiftB;																	\
		if ( !first )																		\
			dataB |= ( old_data[1] >> ( 16 - shiftB ) ) & ( ( 1 << shiftB ) - 1 );			\
	}																						\
																							\
	return ( op );																			\
}

BLIT_FUNCD( 0, ( ~dataA & ~dataB & ~dataC ) ) /* /A/B/C */
BLIT_FUNCD( 1, ( ~dataA & ~dataB &  dataC ) ) /* /A/B C */
BLIT_FUNCD( 2, ( ~dataA &  dataB & ~dataC ) ) /* /A B/C */
BLIT_FUNCD( 3, ( ~dataA &  dataB &  dataC ) ) /* /A B C */
BLIT_FUNCD( 4, (  dataA & ~dataB & ~dataC ) ) /*  A/B/C */
BLIT_FUNCD( 5, (  dataA & ~dataB &  dataC ) ) /*  A/B C */
BLIT_FUNCD( 6, (  dataA &  dataB & ~dataC ) ) /*  A B/C */
BLIT_FUNCD( 7, (  dataA &  dataB &  dataC ) ) /*  A B C */

static int (*blit_func_d[8])( unsigned short *old_data, unsigned short *new_data, int first, int last ) = {
	blit_func_d0,
	blit_func_d1,
	blit_func_d2,
	blit_func_d3,
	blit_func_d4,
	blit_func_d5,
	blit_func_d6,
	blit_func_d7
};

/* General for Ascending/Descending */
static int (**blit_func)( unsigned short *old_data, unsigned short *new_data, int first, int last );

/* Line mode */
#define BLIT_FUNCL( num, op ) static int blit_func_line##num ( unsigned short dataA,		\
															   unsigned short dataB,		\
															   unsigned short dataC ) {		\
	return ( op );																			\
}

BLIT_FUNCL( 0, ( ~dataA & ~dataB & ~dataC ) ) /* /A/B/C */
BLIT_FUNCL( 1, ( ~dataA & ~dataB &  dataC ) ) /* /A/B C */
BLIT_FUNCL( 2, ( ~dataA &  dataB & ~dataC ) ) /* /A B/C */
BLIT_FUNCL( 3, ( ~dataA &  dataB &  dataC ) ) /* /A B C */
BLIT_FUNCL( 4, (  dataA & ~dataB & ~dataC ) ) /*  A/B/C */
BLIT_FUNCL( 5, (  dataA & ~dataB &  dataC ) ) /*  A/B C */
BLIT_FUNCL( 6, (  dataA &  dataB & ~dataC ) ) /*  A B/C */
BLIT_FUNCL( 7, (  dataA &  dataB &  dataC ) ) /*  A B C */

static int (*blit_func_line[8])( unsigned short dataA, unsigned short dataB, unsigned short dataC ) = {
	blit_func_line0,
	blit_func_line1,
	blit_func_line2,
	blit_func_line3,
	blit_func_line4,
	blit_func_line5,
	blit_func_line6,
	blit_func_line7
};

static void blitter_proc( int param ) {
	/* Now we do the real blitting */
	int	blt_total = 0;

	CUSTOM_REG(REG_DMACON) |= 0x2000; /* Blit Zero, we modify it later */

	if ( CUSTOM_REG(REG_BLTCON1) & 1 ) { /* Line mode */
		int linesize, single_bit, single_flag, texture, sign, start, ptr[4], temp_ptr3;
		unsigned short dataA, dataB;

		if ( ( CUSTOM_REG(REG_BLTSIZE) & 0x003f ) != 0x0002 ) {
			logerror("Blitter: BLTSIZE.w != 2 in line mode!\n" );
		}

		if ( ( CUSTOM_REG(REG_BLTCON0) & 0x0b00 ) != 0x0b00 ) {
			logerror("Blitter: Channel selection incorrect in line mode!\n" );
		}

		linesize = ( CUSTOM_REG(REG_BLTSIZE) >> 6 ) & 0x3ff;
		if ( linesize == 0 )
			linesize = 0x400;

		single_bit = CUSTOM_REG(REG_BLTCON1) & 0x0002;
		single_flag = 0;
		sign = CUSTOM_REG(REG_BLTCON1) & 0x0040;
		texture = ( CUSTOM_REG(REG_BLTCON1) >> 12 ) & 0x0f;
		start = ( CUSTOM_REG(REG_BLTCON0) >> 12 ) & 0x0f;

		dataA = ( CUSTOM_REG(REG_BLTADAT) >> start );

		ptr[0] = CUSTOM_REG_LONG(REG_BLTAPTH);
		ptr[2] = CUSTOM_REG_LONG(REG_BLTCPTH);
		ptr[3] = CUSTOM_REG_LONG(REG_BLTDPTH);

		dataB = ( CUSTOM_REG(REG_BLTBDAT) >> texture ) | ( CUSTOM_REG(REG_BLTBDAT) << ( 16 - texture ) );

		while ( linesize-- ) {
			int dst_data = 0;
			int i;

			temp_ptr3 = ptr[3];

			if ( CUSTOM_REG(REG_BLTCON0) & 0x0200 )
				CUSTOM_REG(REG_BLTCDAT) = amiga_chip_ram[ptr[2]/2];

			dataA = ( CUSTOM_REG(REG_BLTADAT) >> start );

			if ( single_bit && single_flag ) dataA = 0;
			single_flag = 1;

			for ( i = 0; i < 8; i++ ) {
				if ( CUSTOM_REG(REG_BLTCON0) & ( 1 << i ) ) {
					dst_data |= (*blit_func_line[i])( dataA, ( dataB & 1 ) * 0xffff, CUSTOM_REG(REG_BLTCDAT) );
				}
			}

			if ( !sign ) {
				ptr[0] += CUSTOM_REG_SIGNED(REG_BLTAMOD);
				if ( CUSTOM_REG(REG_BLTCON1) & 0x0010 ) {
					if ( CUSTOM_REG(REG_BLTCON1) & 0x0008 ) { /* Decrement Y */
						ptr[2] -= CUSTOM_REG_SIGNED(REG_BLTCMOD);
						ptr[3] -= CUSTOM_REG_SIGNED(REG_BLTCMOD); /* ? */
						single_flag = 0;
					} else { /* Increment Y */
						ptr[2] += CUSTOM_REG_SIGNED(REG_BLTCMOD);
						ptr[3] += CUSTOM_REG_SIGNED(REG_BLTCMOD); /* ? */
						single_flag = 0;
					}
				} else {
					if ( CUSTOM_REG(REG_BLTCON1) & 0x0008 ) { /* Decrement X */
						if ( start-- == 0 ) {
							start = 15;
							ptr[2] -= 2;
							ptr[3] -= 2;
						}
					} else { /* Increment X */
						if ( ++start == 16 ) {
							start = 0;
							ptr[2] += 2;
							ptr[3] += 2;
						}
					}
				}
			} else
				ptr[0] += CUSTOM_REG_SIGNED(REG_BLTBMOD);

			if ( CUSTOM_REG(REG_BLTCON1) & 0x0010 ) {
				if ( CUSTOM_REG(REG_BLTCON1) & 0x0004 ) { /* Decrement X */
					if ( start-- == 0 ) {
						start = 15;
						ptr[2] -= 2;
						ptr[3] -= 2;
					}
				} else {
					if ( ++start == 16 ) { /* Increment X */
						start = 0;
						ptr[2] += 2;
						ptr[3] += 2;
					}
				}
			} else {
				if ( CUSTOM_REG(REG_BLTCON1) & 0x0004 ) { /* Decrement Y */
					ptr[2] -= CUSTOM_REG_SIGNED(REG_BLTCMOD);
					ptr[3] -= CUSTOM_REG_SIGNED(REG_BLTCMOD); /* ? */
					single_flag = 0;
				} else { /* Increment Y */
					ptr[2] += CUSTOM_REG_SIGNED(REG_BLTCMOD);
					ptr[3] += CUSTOM_REG_SIGNED(REG_BLTCMOD); /* ? */
					single_flag = 0;
				}
			}

			sign = 0 > ( signed short )ptr[0];

			blt_total |= dst_data;

			if ( CUSTOM_REG(REG_BLTCON0) & 0x0100 )
				amiga_chip_ram[temp_ptr3/2] = dst_data;

			dataB = ( dataB << 1 ) | ( dataB >> 15 );
		}

		CUSTOM_REG_LONG(REG_BLTAPTH) = ptr[0] & 0x1fffff;
		CUSTOM_REG_LONG(REG_BLTCPTH) = ptr[2] & 0x1fffff;
		CUSTOM_REG_LONG(REG_BLTDPTH) = ptr[3] & 0x1fffff;

//      CUSTOM_REG(REG_BLTADAT) = dataA;
//      CUSTOM_REG(REG_BLTBDAT) = dataB;

	} else { /* Blit mode */
		if ( CUSTOM_REG(REG_BLTCON0) & 0x0f00 ) {
			int i, x, y;
			int ptr[4] = { -1, -1, -1, -1 };
			int width = ( CUSTOM_REG(REG_BLTSIZE) & 0x3f );
			int height = ( ( CUSTOM_REG(REG_BLTSIZE) >> 6 ) & 0x3ff );
			unsigned short old_data[3];
			unsigned short new_data[3];
			int fc = 0;

			if ( CUSTOM_REG(REG_BLTCON0) & 0x800 )
				ptr[0] = CUSTOM_REG_LONG(REG_BLTAPTH);

			if ( CUSTOM_REG(REG_BLTCON0) & 0x400 )
				ptr[1] = CUSTOM_REG_LONG(REG_BLTBPTH);

			if ( CUSTOM_REG(REG_BLTCON0) & 0x200 )
				ptr[2] = CUSTOM_REG_LONG(REG_BLTCPTH);

			if ( CUSTOM_REG(REG_BLTCON0) & 0x100 )
			{
				ptr[3] = CUSTOM_REG_LONG(REG_BLTDPTH);
				if ( ptr[3] > 0x7ffff )
					goto done;
			}

			if ( CUSTOM_REG(REG_BLTCON1) & 0x0002 ) { /* Descending mode */
				blit_func = blit_func_d;
				fc = ( CUSTOM_REG(REG_BLTCON1) >> 2 ) & 1; /* fill carry setup */
			} else {
				blit_func = blit_func_a;
			}

			if ( width == 0 )
				width = 0x40;

			if ( height == 0 )
				height = 0x400;

			for ( y = 0; y < height; y++ ) {
				fc = ( CUSTOM_REG(REG_BLTCON1) >> 2 ) & 1; /* fill carry setup */
				for ( x = 0; x < width; x++ ) {
					int dst_data = 0;

					/* get old data */
					new_data[0] = old_data[0] = CUSTOM_REG(REG_BLTADAT);
					new_data[1] = old_data[1] = CUSTOM_REG(REG_BLTBDAT);
					new_data[2] = old_data[2] = CUSTOM_REG(REG_BLTCDAT);

					/* get new data */
					if ( ptr[0] != -1 )
						new_data[0] = amiga_chip_ram[ptr[0]/2];

					if ( ptr[1] != -1 )
						new_data[1] = amiga_chip_ram[ptr[1]/2];

					if ( ptr[2] != -1 )
						new_data[2] = amiga_chip_ram[ptr[2]/2];

					for ( i = 0; i < 8; i++ ) {
						if ( CUSTOM_REG(REG_BLTCON0) & ( 1 << i ) ) {
							dst_data |= (*blit_func[i])( old_data, new_data, x == 0, x == (width - 1) );
						}
					}

					/* store new data */
					CUSTOM_REG(REG_BLTADAT) = new_data[0];
					CUSTOM_REG(REG_BLTBDAT) = new_data[1];
					CUSTOM_REG(REG_BLTCDAT) = new_data[2];

					if ( CUSTOM_REG(REG_BLTCON1) & 0x0002 ) { /* Descending mode */
						if ( CUSTOM_REG(REG_BLTCON1) & 0x0018 ) /* Fill mode */
							dst_data = blitter_fill( dst_data, CUSTOM_REG(REG_BLTCON1) & 0x18, &fc );

						if ( ptr[3] != -1 ) {
							amiga_chip_ram[ptr[3]/2] = dst_data;
							ptr[3] -= 2;
						}

						if ( ptr[0] != -1 )
							ptr[0] -= 2;

						if ( ptr[1] != -1 )
							ptr[1] -= 2;

						if ( ptr[2] != -1 )
							ptr[2] -= 2;
					} else {
						if ( ptr[3] != -1 ) {
							amiga_chip_ram[ptr[3]/2] = dst_data;
							ptr[3] += 2;
						}

						if ( ptr[0] != -1 )
							ptr[0] += 2;

						if ( ptr[1] != -1 )
							ptr[1] += 2;

						if ( ptr[2] != -1 )
							ptr[2] += 2;
					}
					blt_total |= dst_data;
				}

				if ( CUSTOM_REG(REG_BLTCON1) & 0x0002 ) { /* Descending mode */
					if ( ptr[0] != -1 )
						ptr[0] -= CUSTOM_REG_SIGNED(REG_BLTAMOD);

					if ( ptr[1] != -1 )
						ptr[1] -= CUSTOM_REG_SIGNED(REG_BLTBMOD);

					if ( ptr[2] != -1 )
						ptr[2] -= CUSTOM_REG_SIGNED(REG_BLTCMOD);

					if ( ptr[3] != -1 )
						ptr[3] -= CUSTOM_REG_SIGNED(REG_BLTDMOD);
				} else {
					if ( ptr[0] != -1 )
						ptr[0] += CUSTOM_REG_SIGNED(REG_BLTAMOD);

					if ( ptr[1] != -1 )
						ptr[1] += CUSTOM_REG_SIGNED(REG_BLTBMOD);

					if ( ptr[2] != -1 )
						ptr[2] += CUSTOM_REG_SIGNED(REG_BLTCMOD);

					if ( ptr[3] != -1 )
						ptr[3] += CUSTOM_REG_SIGNED(REG_BLTDMOD);
				}
			}

			/* We're done, update the ptr's now */
			if ( ptr[0] != -1 ) {
				CUSTOM_REG_LONG(REG_BLTAPTH) = ptr[0];
			}

			if ( ptr[1] != -1 ) {
				CUSTOM_REG_LONG(REG_BLTBPTH) = ptr[1];
			}

			if ( ptr[2] != -1 ) {
				CUSTOM_REG_LONG(REG_BLTCPTH) = ptr[2];
			}

			if ( ptr[3] != -1 ) {
				CUSTOM_REG_LONG(REG_BLTDPTH) = ptr[3];
			}
		}
	}

done:
	CUSTOM_REG(REG_DMACON) ^= 0x4000; /* signal we're done */

	if ( blt_total )
		CUSTOM_REG(REG_DMACON) ^= 0x2000;


	amiga_custom_w(REG_INTREQ, 0x8040, 0);
}

static void blitter_setup( void ) {
	int ticks, width, height;
	double blit_time;

	if ( ( CUSTOM_REG(REG_DMACON) & 0x0240 ) != 0x0240 ) /* Enabled ? */
		return;

	if ( CUSTOM_REG(REG_DMACON) & 0x4000 ) { /* Is there another blitting in progress? */
		logerror("This program is playing tricks with the blitter\n" );
		return;
	}

	if ( CUSTOM_REG(REG_BLTCON1) & 1 ) { /* Line mode */
		ticks = 8;
	} else {
		ticks = 4;
		if ( CUSTOM_REG(REG_BLTCON0) & 0x0400 ) /* channel B, +2 ticks */
			ticks += 2;
		if ( ( CUSTOM_REG(REG_BLTCON0) & 0x0300 ) == 0x0300 ) /* Both channel C and D, +2 ticks */
			ticks += 2;
	}

	CUSTOM_REG(REG_DMACON) |= 0x4000; /* signal blitter busy */

	width = ( CUSTOM_REG(REG_BLTSIZE) & 0x3f );
	if ( width == 0 )
		width = 0x40;

	height = ( ( CUSTOM_REG(REG_BLTSIZE) >> 6 ) & 0x3ff );
	if ( height == 0 )
		height = 0x400;

	blit_time = ( (double)ticks * (double)width * (double)height );

	blit_time /= ( (double)Machine->drv->cpu[0].cpu_clock / 1000000.0 );

	timer_set( TIME_IN_USEC( blit_time ), 0, blitter_proc );
}

/***************************************************************************

    8520 CIA emulation

***************************************************************************/

/* required prototype */
static void cia_fire_timer( int cia, int timer );
static mame_timer *cia_hblank_timer;
static int cia_hblank_timer_set;

typedef struct {
	unsigned char 	ddra;
	unsigned char 	ddrb;
	int	(*portA_read)( void );
	int	(*portB_read)( void );
	void (*portA_write)( int data );
	void (*portB_write)( int data );
	unsigned char	data_latchA;
	unsigned char	data_latchB;
	/* Timer A */
	unsigned short	timerA_latch;
	unsigned short	timerA_count;
	unsigned char	timerA_mode;
	/* Timer B */
	unsigned short	timerB_latch;
	unsigned short	timerB_count;
	unsigned char	timerB_mode;
	/* Time Of the Day clock (TOD) */
	unsigned long	tod;
	int				tod_running;
	unsigned long	alarm;
	/* Interrupts */
	int				icr;
	int				ics;
	/* MESS timers */
	mame_timer		*timerA;
	mame_timer		*timerB;
	int				timerA_started;
	int				timerB_started;
} cia_8520_def;

static cia_8520_def cia_8520[2];

static void cia_timer_proc( int param ) {
	int cia = param >> 8;
	int timer = param & 1;

	if ( timer == 0 )
		cia_8520[cia].timerA_started = 0;
	else
		cia_8520[cia].timerB_started = 0;

	if ( timer == 0 ) {
		if ( cia_8520[cia].timerA_mode & 0x08 ) { /* One shot */
			cia_8520[cia].ics |= 0x81; /* set status */
			if ( cia_8520[cia].icr & 1 ) {
				if ( cia == 0 ) {
					amiga_custom_w(REG_INTREQ, 0x8008, 0);
				} else {
					amiga_custom_w(REG_INTREQ, 0xa000, 0);
				}
			}
			cia_8520[cia].timerA_count = cia_8520[cia].timerA_latch; /* Reload the timer */
			cia_8520[cia].timerA_mode &= 0xfe; /* Shut it down */
		} else { /* Continuous */
			cia_8520[cia].ics |= 0x81; /* set status */
			if ( cia_8520[cia].icr & 1 ) {
				if ( cia == 0 ) {
					amiga_custom_w(REG_INTREQ, 0x8008, 0);
				} else {
					amiga_custom_w(REG_INTREQ, 0xa000, 0);
				}
			}
			cia_8520[cia].timerA_count = cia_8520[cia].timerA_latch; /* Reload the timer */
			cia_fire_timer( cia, 0 ); /* keep going */
		}
	} else {
		if ( cia_8520[cia].timerB_mode & 0x08 ) { /* One shot */
			cia_8520[cia].ics |= 0x82; /* set status */
			if ( cia_8520[cia].icr & 2 ) {
				if ( cia == 0 ) {
					amiga_custom_w(REG_INTREQ, 0x8008, 0);
				} else {
					amiga_custom_w(REG_INTREQ, 0xa000, 0);
				}
			}
			cia_8520[cia].timerB_count = cia_8520[cia].timerB_latch; /* Reload the timer */
			cia_8520[cia].timerB_mode &= 0xfe; /* Shut it down */
		} else { /* Continuous */
			cia_8520[cia].ics |= 0x82; /* set status */
			if ( cia_8520[cia].icr & 2 ) {
				if ( cia == 0 ) {
					amiga_custom_w(REG_INTREQ, 0x8008, 0);
				} else {
					amiga_custom_w(REG_INTREQ, 0xa000, 0);
				}
			}
			cia_8520[cia].timerB_count = cia_8520[cia].timerB_latch; /* Reload the timer */
			cia_fire_timer( cia, 1 ); /* keep going */
		}
	}
}

static int cia_get_count( int cia, int timer ) {
	int time;

	/* 715909 Hz for NTSC, 709379 for PAL */

	if ( timer == 0 ) {
		if ( cia_8520[cia].timerA_started )
			time = cia_8520[cia].timerA_count - ( int )( timer_timeelapsed( cia_8520[cia].timerA ) / TIME_IN_HZ( 715909 ) );
		else
			time = cia_8520[cia].timerA_count;
	} else {
		if ( cia_8520[cia].timerB )
			time = cia_8520[cia].timerB_count - ( int )( timer_timeelapsed( cia_8520[cia].timerB ) / TIME_IN_HZ( 715909 ) );
		else
			time = cia_8520[cia].timerB_count;
	}

	return time;
}

static void cia_stop_timer( int cia, int timer ) {

	if ( timer == 0 ) {
		timer_reset( cia_8520[cia].timerA, TIME_NEVER );
		cia_8520[cia].timerA_started = 0;
	} else {
		timer_reset( cia_8520[cia].timerB, TIME_NEVER );
		cia_8520[cia].timerB_started = 0;
	}
}

static void cia_fire_timer( int cia, int timer ) {

	/* 715909 Hz for NTSC, 709379 for PAL */

	if ( timer == 0 ) {
		if ( cia_8520[cia].timerA_started == 0 )
			timer_adjust(cia_8520[cia].timerA, ( double )cia_8520[cia].timerA_count * TIME_IN_HZ( 715909 ), ( cia << 8 ) | timer, 0 );
	} else {
		if ( cia_8520[cia].timerB_started == 0 )
			timer_adjust(cia_8520[cia].timerB, ( double )cia_8520[cia].timerB_count * TIME_IN_HZ( 715909 ), ( cia << 8 ) | timer, 0 );
	}
}

/* Update TOD on CIA A */
static void cia_vblank_update( void ) {
	if ( cia_8520[0].tod_running ) {
		cia_8520[0].tod++;
		if ( cia_8520[0].tod == cia_8520[0].alarm ) {
			cia_8520[0].ics |= 0x84;
			if ( cia_8520[0].icr & 0x04 ) {
				amiga_custom_w(REG_INTREQ, 0x8008, 0);
			}
		}
	}
}

/* Update TOD on CIA B (each hblank) */
static void cia_hblank_update( int param ) {
	if ( cia_8520[1].tod_running ) {
		int i;

		for ( i = 0; i < Machine->drv->screen_height; i++ ) {
			cia_8520[1].tod++;
			if ( cia_8520[1].tod == cia_8520[1].alarm ) {
				cia_8520[1].ics |= 0x84;
				if ( cia_8520[1].icr & 0x04 ) {
				    amiga_custom_w(REG_INTREQ, 0xa000, 0 /* could also be hibyte only 0xff */);
				}
			}
		}
	}

	timer_adjust( cia_hblank_timer, cpu_getscanlineperiod(), 0, 0 );
}

/* Issue a index pulse when a disk revolution completes */
void amiga_cia_issue_index( void ) {
	cia_8520[1].ics |= 0x90;
	if ( cia_8520[1].icr & 0x10 ) {
	    amiga_custom_w(REG_INTREQ, 0xa000, 0 /* could also be hibyte only 0xff*/);
	}
}

static int cia_0_portA_r( void )
{
	return (amiga_intf->cia_0_portA_r) ? amiga_intf->cia_0_portA_r() : 0x00;
}

static int cia_0_portB_r( void )
{
	return (amiga_intf->cia_0_portB_r) ? amiga_intf->cia_0_portB_r() : 0x00;
}

static void cia_0_portA_w( int data )
{
	if (amiga_intf->cia_0_portA_w)
		amiga_intf->cia_0_portA_w(data);
}

static void cia_0_portB_w( int data )
{
	if (amiga_intf->cia_0_portB_w)
		amiga_intf->cia_0_portB_w(data);
}

static int cia_1_portA_r( void )
{
	return (amiga_intf->cia_1_portA_r) ? amiga_intf->cia_1_portA_r() : 0x00;
}

static int cia_1_portB_r( void )
{
	return (amiga_intf->cia_1_portB_r) ? amiga_intf->cia_1_portB_r() : 0x00;
}

static void cia_1_portA_w( int data )
{
	if (amiga_intf->cia_1_portA_w)
		amiga_intf->cia_1_portA_w(data);
}

static void cia_1_portB_w( int data )
{
	if (amiga_intf->cia_1_portB_w)
		amiga_intf->cia_1_portB_w(data);
}

static void cia_init( void ) {
	int i;

	cia_hblank_timer = timer_alloc( cia_hblank_update );
	cia_hblank_timer_set = 0;

	/* Initialize port handlers */
	cia_8520[0].portA_read = cia_0_portA_r;
	cia_8520[0].portB_read = cia_0_portB_r;
	cia_8520[0].portA_write = cia_0_portA_w;
	cia_8520[0].portB_write = cia_0_portB_w;
	cia_8520[1].portA_read = cia_1_portA_r;
	cia_8520[1].portB_read = cia_1_portB_r;
	cia_8520[1].portA_write = cia_1_portA_w;
	cia_8520[1].portB_write = cia_1_portB_w;

	/* Initialize data direction registers */
	cia_8520[0].ddra = 0x03;
	cia_8520[0].ddrb = 0x00; /* undefined */
	cia_8520[1].ddra = 0xff;
	cia_8520[1].ddrb = 0xff;

	/* Initialize timer's, TOD's and interrupts */
	for ( i = 0; i < 2; i++ ) {
		cia_8520[i].data_latchA = 0;
		cia_8520[i].data_latchB = 0;
		cia_8520[i].timerA_latch = 0xffff;
		cia_8520[i].timerA_count = 0;
		cia_8520[i].timerA_mode = 0;
		cia_8520[i].timerB_latch = 0xffff;
		cia_8520[i].timerB_count = 0;
		cia_8520[i].timerB_mode = 0;
		cia_8520[i].tod = 0;
		cia_8520[i].tod_running = 0;
		cia_8520[i].alarm = 0;
		cia_8520[i].icr = 0;
		cia_8520[i].ics = 0;
		cia_8520[i].timerA = timer_alloc( cia_timer_proc );
		cia_8520[i].timerB = timer_alloc( cia_timer_proc );
		cia_8520[i].timerA_started = 0;
		cia_8520[i].timerB_started = 0;
	}
}

READ16_HANDLER ( amiga_cia_r ) {
	int cia_sel = 1, mask, data;

	offset<<=1; //PeT offset with new memory system now in word counts!
	if ( offset >= 0x1000 )
		cia_sel = 0;

	switch( offset & 0xf00 ) {
		case 0x000:
			data = (*cia_8520[cia_sel].portA_read)();
			mask = ~( cia_8520[cia_sel].ddra );
			data &= mask;
			mask = cia_8520[cia_sel].ddra & cia_8520[cia_sel].data_latchA;
			data |= mask;
			return ( cia_sel == 0 ) ? data : ( data << 8 );
		break;

		case 0x100:
			data = (*cia_8520[cia_sel].portB_read)();
			mask = ~( cia_8520[cia_sel].ddrb );
			data &= mask;
			mask = cia_8520[cia_sel].ddrb & cia_8520[cia_sel].data_latchB;
			data |= mask;
			return ( cia_sel == 0 ) ? data : ( data << 8 );
		break;

		case 0x200:
			data = cia_8520[cia_sel].ddra;
			return ( cia_sel == 0 ) ? data : ( data << 8 );
		break;

		case 0x300:
			data = cia_8520[cia_sel].ddrb;
			return ( cia_sel == 0 ) ? data : ( data << 8 );
		break;

		case 0x400:
			data = cia_get_count( cia_sel, 0 ) & 0xff;
			return ( cia_sel == 0 ) ? data : ( data << 8 );
		break;

		case 0x500:
			data = ( cia_get_count( cia_sel, 0 ) >> 8 ) & 0xff;
			return ( cia_sel == 0 ) ? data : ( data << 8 );
		break;

		case 0x600:
			data = cia_get_count( cia_sel, 1 ) & 0xff;
			return ( cia_sel == 0 ) ? data : ( data << 8 );
		break;

		case 0x700:
			data = ( cia_get_count( cia_sel, 1 ) >> 8 ) & 0xff;
			return ( cia_sel == 0 ) ? data : ( data << 8 );
		break;

		case 0x800:
			data = cia_8520[cia_sel].tod & 0xff;
			return ( cia_sel == 0 ) ? data : ( data << 8 );
		break;

		case 0x900:
			data = ( cia_8520[cia_sel].tod >> 8 ) & 0xff;
			return ( cia_sel == 0 ) ? data : ( data << 8 );
		break;

		case 0xa00:
			data = ( cia_8520[cia_sel].tod >> 16 ) & 0xff;
			return ( cia_sel == 0 ) ? data : ( data << 8 );
		break;

		case 0xd00:
			data = cia_8520[cia_sel].ics;
			cia_8520[cia_sel].ics = 0; /* clear on read */
			return ( cia_sel == 0 ) ? data : ( data << 8 );
		break;

		case 0xe00:
			data = cia_8520[cia_sel].timerA_mode;
			return ( cia_sel == 0 ) ? data : ( data << 8 );
		break;

		case 0xf00:
			data = cia_8520[cia_sel].timerB_mode;
			return ( cia_sel == 0 ) ? data : ( data << 8 );
		break;
	}

#if LOG_CIA
	logerror("PC = %06x - Read from CIA %01x\n", activecpu_get_pc(), cia_sel );
#endif

	return 0;
}

WRITE16_HANDLER ( amiga_cia_w ) {
	int cia_sel = 1, mask;

	offset<<=1;
	if ( offset >= 0x1000 )
		cia_sel = 0;
	else
		data >>= 8;

	data &= 0xff;

	switch ( offset & 0xffe ) {
		case 0x000:
			mask = cia_8520[cia_sel].ddra;
			cia_8520[cia_sel].data_latchA = data;
			(*cia_8520[cia_sel].portA_write)( data & mask );
		break;

		case 0x100:
			mask = cia_8520[cia_sel].ddrb;
			cia_8520[cia_sel].data_latchB = data;
			(*cia_8520[cia_sel].portB_write)( data & mask );
		break;

		case 0x200:
			cia_8520[cia_sel].ddra = data;
		break;

		case 0x300:
			cia_8520[cia_sel].ddrb = data;
		break;

		case 0x400:
			cia_8520[cia_sel].timerA_latch &= 0xff00;
			cia_8520[cia_sel].timerA_latch |= data;
		break;

		case 0x500:
			cia_8520[cia_sel].timerA_latch &= 0x00ff;
			cia_8520[cia_sel].timerA_latch |= data << 8;

			/* If it's one shot, start the timer */
			if ( cia_8520[cia_sel].timerA_mode & 0x08 ) {
				cia_8520[cia_sel].timerA_count = cia_8520[cia_sel].timerA_latch;
				cia_8520[cia_sel].timerA_mode |= 1;
				cia_fire_timer( cia_sel, 0 );
			}
		break;

		case 0x600:
			cia_8520[cia_sel].timerB_latch &= 0xff00;
			cia_8520[cia_sel].timerB_latch |= data;
		break;

		case 0x700:
			cia_8520[cia_sel].timerB_latch &= 0x00ff;
			cia_8520[cia_sel].timerB_latch |= data << 8;

			/* If it's one shot, start the timer */
			if ( cia_8520[cia_sel].timerB_mode & 0x08 ) {
				cia_8520[cia_sel].timerB_count = cia_8520[cia_sel].timerB_latch;
				cia_8520[cia_sel].timerB_mode |= 1;
				cia_fire_timer( cia_sel, 1 );
			}
		break;

		case 0x800:
			if ( cia_8520[cia_sel].timerB_mode & 0x80 ) { /* set alarm? */
				cia_8520[cia_sel].alarm &= 0xffff00;
				cia_8520[cia_sel].alarm |= data;
			} else {
				cia_8520[cia_sel].tod &= 0xffff00;
				cia_8520[cia_sel].tod |= data;
				cia_8520[cia_sel].tod_running = 1;
			}
		break;

		case 0x900:
			if ( cia_8520[cia_sel].timerB_mode & 0x80 ) { /* set alarm? */
				cia_8520[cia_sel].alarm &= 0xff00ff;
				cia_8520[cia_sel].alarm |= data << 8;
			} else {
				cia_8520[cia_sel].tod &= 0xff00ff;
				cia_8520[cia_sel].tod |= data << 8;
				cia_8520[cia_sel].tod_running = 0;
			}
		break;

		case 0xa00:
			if ( cia_8520[cia_sel].timerB_mode & 0x80 ) { /* set alarm? */
				cia_8520[cia_sel].alarm &= 0x00ffff;
				cia_8520[cia_sel].alarm |= data << 16;
			} else {
				cia_8520[cia_sel].tod &= 0x00ffff;
				cia_8520[cia_sel].tod |= data << 16;
				cia_8520[cia_sel].tod_running = 0;
			}
		break;

		case 0xd00:
			if ( data & 0x80 ) /* set */
				cia_8520[cia_sel].icr |= ( data & 0x7f );
			else /* clear */
				cia_8520[cia_sel].icr &= ~( data & 0x7f );
		break;

		case 0xe00:
			if ( data & 0x10 ) { /* force load */
				cia_8520[cia_sel].timerA_count = cia_8520[cia_sel].timerA_latch;
				cia_stop_timer( cia_sel, 0 );
			}

			if ( data & 0x01 )
				cia_fire_timer( cia_sel, 0 );
			else
				cia_stop_timer( cia_sel, 0 );

			cia_8520[cia_sel].timerA_mode = data & 0xef;
		break;

		case 0xf00:
			if ( data & 0x10 ) { /* force load */
				cia_8520[cia_sel].timerB_count = cia_8520[cia_sel].timerB_latch;
				cia_stop_timer( cia_sel, 1 );
			}

			if ( data & 0x01 )
				cia_fire_timer( cia_sel, 1 );
			else
				cia_stop_timer( cia_sel, 1 );

			cia_8520[cia_sel].timerB_mode = data & 0xef;
		break;
	}

#if LOG_CIA
	logerror("PC = %06x - Wrote to CIA %01x (%02x)\n", activecpu_get_pc(), cia_sel, data );
#endif
}


/***************************************************************************

    Custom chips emulation

***************************************************************************/

static void amiga_custom_init( void )
{
	CUSTOM_REG(REG_DDFSTRT) = 0x18;
	CUSTOM_REG(REG_DDFSTOP) = 0xd8;
}

READ16_HANDLER( amiga_custom_r )
{
	switch (offset & 0xff)
	{
		case REG_DMACONR:
			return CUSTOM_REG(REG_DMACON);

		case REG_VPOSR:
			return amiga_gethvpos() >> 16;

		case REG_VHPOSR:
			return amiga_gethvpos();

		case REG_JOY0DAT:
			return amiga_intf->read_joy0dat();

		case REG_JOY1DAT:
			return amiga_intf->read_joy1dat();

		case REG_ADKCONR:
			return CUSTOM_REG(REG_ADKCON);

		case REG_POTGOR:
			CUSTOM_REG(REG_POTGOR) = CUSTOM_REG(REG_POTGO) & 0xaa00;
			CUSTOM_REG(REG_POTGOR) |= (readinputport(0) & 1) << 10;
			CUSTOM_REG(REG_POTGOR) |= (readinputport(0) & 2) << 13;
			return CUSTOM_REG(REG_POTGOR);

		case REG_DSKBYTR:
			return amiga_intf->read_dskbytr ? amiga_intf->read_dskbytr() : 0x00;

		case REG_INTENAR:
			return CUSTOM_REG(REG_INTENA);

		case REG_INTREQR:
			return CUSTOM_REG(REG_INTREQ);

		case REG_COPJMP1:
			copper_setpc(CUSTOM_REG_LONG(REG_COP1LCH));
			break;

		case REG_COPJMP2:
			copper_setpc(CUSTOM_REG_LONG(REG_COP2LCH));
			break;

		default:
#if LOG_CUSTOM
			logerror( "PC = %06x - Read from Custom %04x\n", cpu_getactivecpu() != -1 ? activecpu_get_pc() : 0, offset );
#endif
			break;
	}

	return 0;
}


WRITE16_HANDLER( amiga_custom_w )
{
	offset &= 0xff;

	switch (offset)
	{
		case REG_DSKLEN:
			if (amiga_intf->write_dsklen)
				amiga_intf->write_dsklen(data);
			break;

		case REG_COPCON:
			data &= 2;	/* why? */
			break;

		case REG_BLTSIZE:
			CUSTOM_REG(REG_BLTSIZE) = data;
			blitter_setup();
			break;

		case REG_AUD0LCL:	case REG_AUD1LCL:	case REG_AUD2LCL:	case REG_AUD3LCL:
		case REG_BLTCMOD:	case REG_BLTBMOD:	case REG_BLTAMOD:	case REG_BLTDMOD:
		case REG_BLTCPTL:	case REG_BLTBPTL:	case REG_BLTAPTL:	case REG_BLTDPTL:
		case REG_COP1LCL:	case REG_COP2LCL:
		case REG_BPL1PTL:	case REG_BPL2PTL:	case REG_BPL3PTL:	case REG_BPL4PTL:
		case REG_BPL5PTL:	case REG_BPL6PTL:
			/* keep word-aligned */
			data &= 0xfffe;
			break;

		case REG_AUD0LCH:	case REG_AUD1LCH:	case REG_AUD2LCH:	case REG_AUD3LCH:
		case REG_BLTCPTH:	case REG_BLTBPTH:	case REG_BLTAPTH:	case REG_BLTDPTH:
		case REG_COP1LCH:	case REG_COP2LCH:
		case REG_BPL1PTH:	case REG_BPL2PTH:	case REG_BPL3PTH:	case REG_BPL4PTH:
		case REG_BPL5PTH:	case REG_BPL6PTH:
			/* mask off high bits */
			data &= 0x001f;
			break;

		case REG_SPR0PTL:	case REG_SPR1PTL:	case REG_SPR2PTL:	case REG_SPR3PTL:
		case REG_SPR4PTL:	case REG_SPR5PTL:	case REG_SPR6PTL:	case REG_SPR7PTL:
			/* keep word-aligned */
			data &= 0xfffe;
			amiga_sprite_dma_reset((offset - REG_SPR0PTL) / 2);
			break;

		case REG_SPR0PTH:	case REG_SPR1PTH:	case REG_SPR2PTH:	case REG_SPR3PTH:
		case REG_SPR4PTH:	case REG_SPR5PTH:	case REG_SPR6PTH:	case REG_SPR7PTH:
			/* mask off high bits */
			data &= 0x001f;
			break;

		case REG_SPR0CTL:	case REG_SPR1CTL:	case REG_SPR2CTL:	case REG_SPR3CTL:
		case REG_SPR4CTL:	case REG_SPR5CTL:	case REG_SPR6CTL:	case REG_SPR7CTL:
			/* disable comparitor on writes here */
			amiga_sprite_enable_comparitor((offset - REG_SPR0CTL) / 4, FALSE);
			break;

		case REG_SPR0DATA:	case REG_SPR1DATA:	case REG_SPR2DATA:	case REG_SPR3DATA:
		case REG_SPR4DATA:	case REG_SPR5DATA:	case REG_SPR6DATA:	case REG_SPR7DATA:
			/* enable comparitor on writes here */
			amiga_sprite_enable_comparitor((offset - REG_SPR0DATA) / 4, TRUE);
			break;

		case REG_COPJMP1:
			copper_setpc(CUSTOM_REG_LONG(REG_COP1LCH));
			break;

		case REG_COPJMP2:
			copper_setpc(CUSTOM_REG_LONG(REG_COP2LCH));
			break;

		case REG_DDFSTRT:
			/* impose hardware limits ( HRM, page 75 ) */
			data &= 0xfc;
			if (data < 0x18)
				data = 0x18;
			break;

		case REG_DDFSTOP:
			/* impose hardware limits ( HRM, page 75 ) */
			data &= 0xfc;
			if (data > 0xd8)
				data = 0xd8;
			break;

		case REG_DMACON:
			logerror("DMACON = %04X\n", offset);
			amiga_audio_w(offset, data, mem_mask);

			/* bits BBUSY (14) and BZERO (13) are read-only */
			data &= 0x9fff;
			data = (data & 0x8000) ? (CUSTOM_REG(offset) | (data & 0x7fff)) : (CUSTOM_REG(offset) & ~(data & 0x7fff));
			CUSTOM_REG(offset) = data;
			break;

		case REG_INTENA:
			data = (data & 0x8000) ? (CUSTOM_REG(offset) | (data & 0x7fff)) : (CUSTOM_REG(offset) & ~(data & 0x7fff));
			CUSTOM_REG(offset) = data;
			check_ints();
			break;

		case REG_INTREQ:
			data = (data & 0x8000) ? (CUSTOM_REG(offset) | (data & 0x7fff)) : (CUSTOM_REG(offset) & ~(data & 0x7fff));
			CUSTOM_REG(offset) = data;
			check_ints();
			break;

		case REG_ADKCON:
			data = (data & 0x8000) ? (CUSTOM_REG(offset) | (data & 0x7fff)) : (CUSTOM_REG(offset) & ~(data & 0x7fff));
			CUSTOM_REG(offset) = data;
			amiga_audio_w(offset, data, mem_mask);
			break;

		case REG_AUD0PER:	case REG_AUD0VOL:	case REG_AUD0DAT:
		case REG_AUD1PER:	case REG_AUD1VOL:	case REG_AUD1DAT:
		case REG_AUD2PER:	case REG_AUD2VOL:	case REG_AUD2DAT:
		case REG_AUD3PER:	case REG_AUD3VOL:	case REG_AUD3DAT:
			amiga_audio_w(offset, data, mem_mask);
			break;

		case REG_BPLCON0:
			if ((data & (BPLCON0_BPU0 | BPLCON0_BPU1 | BPLCON0_BPU2)) == (BPLCON0_BPU0 | BPLCON0_BPU1 | BPLCON0_BPU2))
			{
				/* planes go from 0 to 6, inclusive */
				logerror( "This game is doing funky planes stuff. (planes > 6)\n" );
				data &= ~BPLCON0_BPU0;
			}
			break;

		case REG_BPLCON1:
			data &= 0xff;
			break;

		case REG_BPLCON2:
			data &= 0x7f;
			break;

		case REG_COLOR00:	case REG_COLOR01:	case REG_COLOR02:	case REG_COLOR03:
		case REG_COLOR04:	case REG_COLOR05:	case REG_COLOR06:	case REG_COLOR07:
		case REG_COLOR08:	case REG_COLOR09:	case REG_COLOR10:	case REG_COLOR11:
		case REG_COLOR12:	case REG_COLOR13:	case REG_COLOR14:	case REG_COLOR15:
		case REG_COLOR16:	case REG_COLOR17:	case REG_COLOR18:	case REG_COLOR19:
		case REG_COLOR20:	case REG_COLOR21:	case REG_COLOR22:	case REG_COLOR23:
		case REG_COLOR24:	case REG_COLOR25:	case REG_COLOR26:	case REG_COLOR27:
		case REG_COLOR28:	case REG_COLOR29:	case REG_COLOR30:	case REG_COLOR31:
			data &= 0xfff;
			CUSTOM_REG(offset + 32) = (data >> 1) & 0x777;
			break;

		default:
#if LOG_CUSTOM
//          logerror("PC = %06x - Wrote to Custom %04x (%04x)\n", safe_activecpu_get_pc(), offset * 2, data);
#endif
			break;
	}

	if (offset <= REG_COLOR31)
		CUSTOM_REG(offset) = data;
}

/***************************************************************************

    Interrupt handling

***************************************************************************/

INTERRUPT_GEN( amiga_vblank_irq )
{
	/* Update TOD on CIA A */
	cia_vblank_update();

	if ( cia_hblank_timer_set == 0 )
	{
		timer_adjust( cia_hblank_timer, cpu_getscanlineperiod(), 0, 0 );
		cia_hblank_timer_set = 1;
	}

	amiga_custom_w(REG_INTREQ, 0x8020, 0);

	if (amiga_intf->interrupt_callback)
		amiga_intf->interrupt_callback();
}

INTERRUPT_GEN( amiga_irq )
{
	int scanline = 261 - cpu_getiloops();

	if ( scanline == 0 )
	{
		/* vblank start */
		amiga_prepare_frame();
		amiga_vblank_irq();
	}

	amiga_render_scanline(scanline);

	/* force a sound update */
	amiga_audio_w(0,0,0);
}

/***************************************************************************

    Init Machine

***************************************************************************/

void amiga_m68k_reset( void )
{
	logerror( "Executed RESET at PC=%06x\n", activecpu_get_pc() );
	amiga_cia_w( 0x1001/2, 1, 0 );	/* enable overlay */
	if ( activecpu_get_pc() < 0x80000 )
	{
		memory_set_opbase(0);
	}
}

MACHINE_RESET(amiga)
{
	/* set m68k reset  function */
	cpunum_set_info_fct(0, CPUINFO_PTR_M68K_RESET_CALLBACK, (genf *)amiga_m68k_reset);

	/* Initialize the CIA's */
	cia_init();

	/* Initialize the Custom chips */
	amiga_custom_init();

	/* set the overlay bit */
	amiga_cia_w( 0x1001/2, 1, 0 );

	if (amiga_intf->reset_callback)
		amiga_intf->reset_callback();
}

void amiga_machine_config(const struct amiga_machine_interface *intf)
{
	amiga_intf = intf;
}



