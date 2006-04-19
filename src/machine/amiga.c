/***************************************************************************

    Amiga Computer / Arcadia Game System

    Driver by:

    Ernesto Corvi & Mariusz Wojcieszek

***************************************************************************/

#include "driver.h"
#include "includes/amiga.h"
#include "cpu/m68000/m68000.h"

#define LOG_CUSTOM	1
#define LOG_CIA		1



/*************************************
 *
 *  Constants
 *
 *************************************/

/* 715909 Hz for NTSC, 709379 for PAL */
#define O2_TIMER_RATE		(TIME_IN_HZ(Machine->drv->cpu[0].cpu_clock / 10))



/*************************************
 *
 *  Type definitions
 *
 *************************************/

typedef struct _cia_timer cia_timer;
typedef struct _cia_port cia_port;
typedef struct _cia_state cia_state;

struct _cia_timer
{
	UINT16		latch;
	UINT16		count;
	UINT8		mode;
	UINT8		started;
	UINT8		irq;
	mame_timer *timer;
	cia_state *	cia;
};

struct _cia_port
{
	UINT8		ddr;
	UINT8		latch;
	int			(*read)(void);
	void		(*write)(int);
};

struct _cia_state
{
	UINT16			irq;

	cia_port		port[2];
	cia_timer		timer[2];

	/* Time Of the Day clock (TOD) */
	UINT32			tod;
	UINT32			tod_latch;
	UINT8			tod_running;
	UINT32			alarm;

	/* Interrupts */
	UINT8			icr;
	UINT8			ics;
};


typedef struct _autoconfig_device autoconfig_device;
struct _autoconfig_device
{
	autoconfig_device *		next;
	amiga_autoconfig_device	device;
	offs_t					base;
};



/*************************************
 *
 *  Globals
 *
 *************************************/

UINT16 *amiga_chip_ram;
size_t amiga_chip_ram_size;
UINT16 *amiga_custom_regs;
UINT16 *amiga_expansion_ram;
UINT16 *amiga_autoconfig_mem;

static cia_state cia_8520[2];
static const struct amiga_machine_interface *amiga_intf;

static autoconfig_device *autoconfig_list;
static autoconfig_device *cur_autoconfig;



/***************************************************************************

    General routines and registers

***************************************************************************/




static void check_ints(void)
{
	int ints = CUSTOM_REG(REG_INTENA) & CUSTOM_REG(REG_INTREQ);
	int irq = -1;

	/* Master interrupt switch */
	if (CUSTOM_REG(REG_INTENA) & 0x4000)
	{
		/* Serial transmit buffer empty, disk block finished, software interrupts */
		if (ints & 0x0007)
			irq = 1;

		/* I/O ports and timer interrupts */
		if (ints & 0x0008)
			irq = 2;

		/* Copper, VBLANK, blitter interrupts */
		if (ints & 0x0070)
			irq = 3;

		/* Audio interrupts */
		if (ints & 0x0780)
			irq = 4;

		/* Serial receive buffer full, disk sync match */
		if (ints & 0x1800)
			irq = 5;

		/* External interrupts */
		if (ints & 0x2000)
			irq = 6;
	}

	/* set the highest IRQ line */
	if (irq >= 0)
		cpunum_set_input_line(0, irq, ASSERT_LINE);
	else
		cpunum_set_input_line(0, 7, CLEAR_LINE);
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
				CUSTOM_REG(REG_BLTCDAT) = amiga_chip_ram_r(ptr[2]);

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
				amiga_chip_ram_w(temp_ptr3, dst_data);

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
						new_data[0] = amiga_chip_ram_r(ptr[0]);

					if ( ptr[1] != -1 )
						new_data[1] = amiga_chip_ram_r(ptr[1]);

					if ( ptr[2] != -1 )
						new_data[2] = amiga_chip_ram_r(ptr[2]);

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
							amiga_chip_ram_w(ptr[3], dst_data);
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
							amiga_chip_ram_w(ptr[3], dst_data);
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


	amiga_custom_w(REG_INTREQ, 0x8000 | INTENA_BLIT, 0);
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

static void cia_update_interrupts(cia_state *cia)
{
	/* always update the high bit of ICS */
	if (cia->ics & 0x7f)
		cia->ics |= 0x80;
	else
		cia->ics &= ~0x80;

	/* based on what is enabled, set/clear the IRQ via the custom chip */
	if (cia->ics & cia->icr)
		amiga_custom_w(REG_INTREQ, 0x8000 | cia->irq, 0);
	else
		amiga_custom_w(REG_INTREQ, 0x0000 | cia->irq, 0);
}


INLINE void cia_timer_start(cia_timer *timer)
{
	if (!timer->started)
		timer_adjust_ptr(timer->timer, (double)timer->count * O2_TIMER_RATE, 0);
}


INLINE void cia_timer_stop(cia_timer *timer)
{
	timer_reset(timer->timer, TIME_NEVER);
	timer->started = FALSE;
}


INLINE int cia_timer_count(cia_timer *timer)
{
	/* based on whether or not the timer is running, return the current count value */
	if (timer->started)
		return timer->count - (int)(timer_timeelapsed(timer->timer) / O2_TIMER_RATE);
	else
		return timer->count;
}


static void cia_timer_proc(void *param)
{
	cia_timer *timer = param;

	/* clear the timer started flag */
	timer->started = FALSE;

	/* set the status and update interrupts */
	timer->cia->ics |= timer->irq;
	cia_update_interrupts(timer->cia);

	/* reload the timer */
	timer->count = timer->latch;

	/* if one-shot mode, turn it off; otherwise, reprime the timer */
	if (timer->mode & 0x08)
		timer->mode &= 0xfe;
	else
		cia_timer_start(timer);
}


/* Update TOD on CIA A */
static void cia_clock_tod(cia_state *cia)
{
	if (cia->tod_running)
	{
		cia->tod++;
		cia->tod &= 0xffffff;
		if (cia->tod == cia->alarm)
		{
			cia->ics |= 0x04;
			cia_update_interrupts(cia);
		}
	}
}


/* Issue a index pulse when a disk revolution completes */
void amiga_cia_issue_index(void)
{
	cia_state *cia = &cia_8520[1];
	cia->ics |= 0x10;
	cia_update_interrupts(cia);
}


static void cia_reset(void)
{
	int i, t;

	/* initialize the CIA states */
	memset(&cia_8520, 0, sizeof(cia_8520));

	/* loop over and set up initial values */
	for (i = 0; i < 2; i++)
	{
		cia_state *cia = &cia_8520[i];

		/* select IRQ bit based on which CIA */
		cia->irq = (i == 0) ? INTENA_PORTS : INTENA_EXTER;

		/* initialize port handlers */
		cia->port[0].read = (i == 0) ? amiga_intf->cia_0_portA_r : amiga_intf->cia_1_portA_r;
		cia->port[1].read = (i == 0) ? amiga_intf->cia_0_portB_r : amiga_intf->cia_1_portB_r;
		cia->port[0].write = (i == 0) ? amiga_intf->cia_0_portA_w : amiga_intf->cia_1_portA_w;
		cia->port[1].write = (i == 0) ? amiga_intf->cia_0_portB_w : amiga_intf->cia_1_portB_w;

		/* initialize data direction registers */
		cia->port[0].ddr = (i == 0) ? 0x03 : 0xff;
		cia->port[1].ddr = (i == 0) ? 0x00 : 0xff;

		/* TOD running by default */
		cia->tod_running = TRUE;

		/* initialize timers */
		for (t = 0; t < 2; t++)
		{
			cia_timer *timer = &cia->timer[t];

			timer->latch = 0xffff;
			timer->irq = 0x01 << t;
			timer->cia = cia;
			timer->timer = timer_alloc_ptr(cia_timer_proc, timer);
		}
	}
}


READ16_HANDLER( amiga_cia_r )
{
	cia_timer *timer;
	cia_state *cia;
	cia_port *port;
	UINT8 data;
	int shift;

	/* offsets 0000-07ff reference CIA B, and are accessed via the MSB */
	if ((offset & 0x0800) == 0)
	{
		cia = &cia_8520[1];
		shift = 8;
	}

	/* offsets 0800-0fff reference CIA A, and are accessed via the LSB */
	else
	{
		cia = &cia_8520[0];
		shift = 0;
	}

	/* switch off the offset */
	switch (offset & 0x780)
	{
		/* port A/B data */
		case CIA_PRA:
		case CIA_PRB:
			port = &cia->port[(offset >> 7) & 1];
			data = port->read ? (*port->read)() : 0;
			data = (data & ~port->ddr) | (port->latch & port->ddr);
			break;

		/* port A/B direction */
		case CIA_DDRA:
		case CIA_DDRB:
			port = &cia->port[(offset >> 7) & 1];
			data = port->ddr;
			break;

		/* timer A/B low byte */
		case CIA_TALO:
		case CIA_TBLO:
			timer = &cia->timer[(offset >> 8) & 1];
			data = cia_timer_count(timer) >> 0;
			break;

		/* timer A/B high byte */
		case CIA_TAHI:
		case CIA_TBHI:
			timer = &cia->timer[(offset >> 8) & 1];
			data = cia_timer_count(timer) >> 8;
			break;

		/* TOD counter low byte */
		case CIA_TODLOW:
			data = cia->tod_latch >> 0;
			break;

		/* TOD counter middle byte */
		case CIA_TODMID:
			data = cia->tod_latch >> 8;
			break;

		/* TOD counter high byte */
		case CIA_TODHI:
			cia->tod_latch = cia->tod;
			data = cia->tod_latch >> 16;
			break;

		/* interrupt status/clear */
		case CIA_ICR:
			data = cia->ics;
			cia->ics = 0; /* clear on read */
			cia_update_interrupts(cia);
			break;

		/* timer A/B mode */
		case CIA_CRA:
		case CIA_CRB:
			timer = &cia->timer[(offset >> 7) & 1];
			data = timer->mode;
			break;
	}

#if LOG_CIA
	logerror("%06x:cia_%c_read(%03x) = %04x & %04x\n", safe_activecpu_get_pc(), 'A' + ((~offset & 0x0800) >> 11), offset * 2, data << shift, mem_mask ^ 0xffff);
#endif

	return data << shift;
}


WRITE16_HANDLER( amiga_cia_w )
{
	cia_timer *timer;
	cia_state *cia;
	cia_port *port;
	int shift;

#if LOG_CIA
	logerror("%06x:cia_%c_write(%03x) = %04x & %04x\n", safe_activecpu_get_pc(), 'A' + ((~offset & 0x0800) >> 11), offset * 2, data, mem_mask ^ 0xffff);
#endif

	/* offsets 0000-07ff reference CIA B, and are accessed via the MSB */
	if ((offset & 0x0800) == 0)
	{
		if (!ACCESSING_MSB)
			return;
		cia = &cia_8520[1];
		data >>= 8;
	}

	/* offsets 0800-0fff reference CIA A, and are accessed via the LSB */
	else
	{
		if (!ACCESSING_LSB)
			return;
		cia = &cia_8520[0];
		data &= 0xff;
	}

	/* handle the writes */
	switch (offset & 0x7ff)
	{
		/* port A/B data */
		case CIA_PRA:
		case CIA_PRB:
			port = &cia->port[(offset >> 7) & 1];
			port->latch = data;
			if (port->write)
				(*port->write)(data & port->ddr);
			break;

		/* port A/B direction */
		case CIA_DDRA:
		case CIA_DDRB:
			port = &cia->port[(offset >> 7) & 1];
			port->ddr = data;
			break;

		/* timer A/B latch low */
		case CIA_TALO:
		case CIA_TBLO:
			timer = &cia->timer[(offset >> 8) & 1];
			timer->latch = (timer->latch & 0xff00) | (data << 0);
			break;

		/* timer A latch high */
		case CIA_TAHI:
		case CIA_TBHI:
			timer = &cia->timer[(offset >> 8) & 1];
			timer->latch = (timer->latch & 0x00ff) | (data << 8);

			/* if it's one shot, start the timer */
			if (timer->mode & 0x08)
			{
				timer->count = timer->latch;
				timer->mode |= 0x01;
				cia_timer_start(timer);
			}
			break;

		/* time of day latches */
		case CIA_TODLOW:
		case CIA_TODMID:
		case CIA_TODHI:
			shift = 8 * ((offset - CIA_TODLOW) >> 7);

			/* alarm setting mode? */
			if (cia->timer[1].mode & 0x80)
				cia->alarm = (cia->alarm & ~(0xff << shift)) | (data << shift);

			/* counter setting mode */
			else
			{
				cia->tod = (cia->tod & ~(0xff << shift)) | (data << shift);

				/* only enable the TOD once the LSB is written */
				cia->tod_running = (shift == 0);
			}
			break;

		/* interrupt control register */
		case CIA_ICR:
			if (data & 0x80)
				cia->icr |= data & 0x7f;
			else
				cia->icr &= ~(data & 0x7f);
			cia_update_interrupts(cia);
			break;

		/* timer A/B modes */
		case CIA_CRA:
		case CIA_CRB:
			timer = &cia->timer[(offset >> 7) & 1];
			timer->mode = data & 0xef;

			if (data & 0x02)
				printf("Timer %c output on PB\n", 'A' + ((offset >> 8) & 1));

			/* force load? */
			if (data & 0x10)
			{
				timer->count = timer->latch;
				cia_timer_stop(timer);
			}

			/* enable/disable? */
			if (data & 0x01)
				cia_timer_start(timer);
			else
				cia_timer_stop(timer);
			break;
	}
}


/***************************************************************************

    Custom chips emulation

***************************************************************************/

static void amiga_custom_reset( void )
{
	CUSTOM_REG(REG_DDFSTRT) = 0x18;
	CUSTOM_REG(REG_DDFSTOP) = 0xd8;
}

READ16_HANDLER( amiga_custom_r )
{
	UINT16 temp;

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

		case REG_CLXDAT:
			printf("CLXDAT read\n");
			temp = CUSTOM_REG(REG_CLXDAT);
			CUSTOM_REG(REG_CLXDAT) = 0;
			return temp;

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
			logerror("PC = %06x - Wrote to Custom %04x (%04x)\n", safe_activecpu_get_pc(), offset * 2, data);

	offset &= 0xff;

	switch (offset)
	{
		case REG_BLTDDAT:	case REG_DMACONR:	case REG_VPOSR:		case REG_VHPOSR:
		case REG_DSKDATR:	case REG_JOY0DAT:	case REG_JOY1DAT:	case REG_CLXDAT:
		case REG_ADKCONR:	case REG_POT0DAT:	case REG_POT1DAT:	case REG_POTGOR:
		case REG_SERDATR:	case REG_DSKBYTR:	case REG_INTENAR:	case REG_INTREQR:
			/* read-only registers */
			break;

		case REG_DSKLEN:
			if (amiga_intf->write_dsklen)
				amiga_intf->write_dsklen(data);
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
			logerror("PC = %06x - Wrote to Custom %04x (%04x)\n", safe_activecpu_get_pc(), offset * 2, data);
#endif
			break;
	}

	if (offset <= REG_COLOR31)
		CUSTOM_REG(offset) = data;
}

/***************************************************************************

    Interrupt handling

***************************************************************************/

INTERRUPT_GEN( amiga_irq )
{
	int scanline = 261 - cpu_getiloops();

	if ( scanline == 0 )
	{
		/* vblank start */
		cia_clock_tod(&cia_8520[0]);

		amiga_custom_w(REG_INTREQ, 0x8000 | INTENA_VERTB, 0);

		if (amiga_intf->interrupt_callback)
			amiga_intf->interrupt_callback();
	}
	cia_clock_tod(&cia_8520[1]);

	amiga_render_scanline(scanline);

	/* force a sound update */
	amiga_audio_w(0,0,0);
}




/***************************************************************************

    Autoconfig devices

***************************************************************************/

void amiga_add_autoconfig(amiga_autoconfig_device *device)
{
	autoconfig_device *dev, **d;

	/* validate the data */
	assert_always((device->size & (device->size - 1)) == 0, "device->size must be power of 2!");

	/* allocate memory and link it in at the end of the list */
	dev = auto_malloc(sizeof(*dev));
	dev->next = NULL;
	for (d = &autoconfig_list; *d; d = &(*d)->next) ;
	*d = dev;

	/* fill in the data */
	dev->device = *device;
	dev->base = 0;
}


static void autoconfig_reset(void)
{
	autoconfig_device *dev;

	/* uninstall any installed devices */
	for (dev = autoconfig_list; dev; dev = dev->next)
		if (dev->base && dev->device.uninstall)
		{
			(*dev->device.uninstall)(dev->base);
			dev->base = 0;
		}

	/* reset the current autoconfig */
	cur_autoconfig = autoconfig_list;
}


READ16_HANDLER( amiga_autoconfig_r )
{
	UINT8 byte;
	int i;

	/* if nothing present, just return */
	if (!cur_autoconfig)
	{
		logerror("autoconfig_r(%02X) but no device selected\n", offset);
		return 0xffff;
	}

	/* switch off of the base offset */
	switch (offset/2)
	{
		/*
           00/02        1  1  x  x     x  0  0  0 = 8 Megabytes
                              ^  ^     ^  0  0  1 = 64 Kbytes
                              |  |     |  0  1  0 = 128 Kbytes
                              |  |     |  0  1  1 = 256 Kbytes
                              |  |     |  1  0  0 = 1 Megabyte
                              |  |     |  1  1  0 = 2 Megabytes
                              |  |     |  1  1  1 = 4 Megabytes
                              |  |     |
                              |  |     `-- 1 = multiple devices on this card
                              |  `-------- 1 = ROM vector offset is valid
                              `----------- 1 = link into free memory list
        */
		case 0x00/4:
			byte = 0xc0;
			if (cur_autoconfig->device.link_memory)
				byte |= 0x20;
			if (cur_autoconfig->device.rom_vector_valid)
				byte |= 0x10;
			if (cur_autoconfig->device.multi_device)
				byte |= 0x08;
			for (i = 0; i < 8; i++)
				if (cur_autoconfig->device.size & (1 << i))
					break;
			byte |= (i + 1) & 7;
			break;

		/*
           04/06          product number (all bits inverted)
        */
		case 0x04/4:
			byte = ~cur_autoconfig->device.product_number;
			break;

		/*
           08/0a        x  x  1  1     1  1  1  1
                        ^  ^
                        |  |
                        |  `-- 1 = this board can be shut up
                        `----- 0 = prefer 8 Meg address space
        */
		case 0x08/4:
			byte = 0x3f;
			if (!cur_autoconfig->device.prefer_8meg)
				byte |= 0x80;
			if (cur_autoconfig->device.can_shutup)
				byte |= 0x40;
			break;

		/*
           10/12         manufacturers number (high byte, all inverted)
           14/16                  ''          (low byte, all inverted)
        */
		case 0x10/4:
			byte = ~cur_autoconfig->device.mfr_number >> 8;
			break;

		case 0x14/4:
			byte = ~cur_autoconfig->device.mfr_number >> 0;
			break;

		/*
           18/1a         optional serial number (all bits inverted) byte0
           1c/1e                              ''                    byte1
           20/22                              ''                    byte2
           24/26                              ''                    byte3
        */
		case 0x18/4:
			byte = ~cur_autoconfig->device.serial_number >> 24;
			break;

		case 0x1c/4:
			byte = ~cur_autoconfig->device.serial_number >> 16;
			break;

		case 0x20/4:
			byte = ~cur_autoconfig->device.serial_number >> 8;
			break;

		case 0x24/4:
			byte = ~cur_autoconfig->device.serial_number >> 0;
			break;

		/*
           28/2a         optional ROM vector offset (all bits inverted) high byte
           2c/2e                              ''                        low byte
        */
		case 0x28/4:
			byte = ~cur_autoconfig->device.rom_vector >> 8;
			break;

		case 0x2c/4:
			byte = ~cur_autoconfig->device.rom_vector >> 0;
			break;

		/*
           40/42   optional interrupt control and status register
        */
		case 0x40/4:
			byte = 0x00;
			if (cur_autoconfig->device.int_control_r)
				byte = (*cur_autoconfig->device.int_control_r)();
			break;

		default:
			byte = 0xff;
			break;
	}

	/* return the appropriate nibble */
	logerror("autoconfig_r(%02X) = %04X\n", offset, (offset & 1) ? ((byte << 12) | 0xfff) : ((byte << 8) | 0xfff));
	return (offset & 1) ? ((byte << 12) | 0xfff) : ((byte << 8) | 0xfff);
}


WRITE16_HANDLER( amiga_autoconfig_w )
{
	int move_to_next = FALSE;

	logerror("autoconfig_w(%02X) = %04X & %04X\n", offset, data, mem_mask ^ 0xffff);

	/* if no current device, bail */
	if (!cur_autoconfig || !ACCESSING_MSB)
		return;

	/* switch off of the base offset */
	switch (offset/2)
	{
		/*
           48/4a        write-only register for base address (A23-A16)
        */
		case 0x48/4:
			if ((offset & 1) == 0)
				cur_autoconfig->base = (cur_autoconfig->base & ~0xf00000) | ((data & 0xf000) << 8);
			else
				cur_autoconfig->base = (cur_autoconfig->base & ~0x0f0000) | ((data & 0xf000) << 4);
			move_to_next = TRUE;
			break;

		/*
           4c/4e        optional write-only 'shutup' trigger
        */
		case 0x4c/4:
			cur_autoconfig->base = 0;
			move_to_next = TRUE;
			break;
	}

	/* install and move to the next device if requested */
	if (move_to_next && (offset & 1) == 0)
	{
		logerror("Install to %06X\n", cur_autoconfig->base);
		if (cur_autoconfig->base && cur_autoconfig->device.install)
			(*cur_autoconfig->device.install)(cur_autoconfig->base);
		cur_autoconfig = cur_autoconfig->next;
	}
}



/***************************************************************************

    Init Machine

***************************************************************************/

void amiga_m68k_reset( void )
{
	logerror("Executed RESET at PC=%06x\n", activecpu_get_pc());
	amiga_cia_w(0x1001/2, 1, 0);	/* enable overlay */
	if (activecpu_get_pc() < 0x80000)
		memory_set_opbase(0);
}

MACHINE_RESET(amiga)
{
	/* set m68k reset  function */
	cpunum_set_info_fct(0, CPUINFO_PTR_M68K_RESET_CALLBACK, (genf *)amiga_m68k_reset);

	/* Initialize the CIA's */
	cia_reset();

	/* Initialize the Custom chips */
	amiga_custom_reset();

	/* set the overlay bit */
	amiga_cia_w(0x1001/2, 1, 0);

	autoconfig_reset();

	if (amiga_intf->reset_callback)
		amiga_intf->reset_callback();
}

void amiga_machine_config(const struct amiga_machine_interface *intf)
{
	amiga_intf = intf;
}

