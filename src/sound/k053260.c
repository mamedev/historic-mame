/*********************************************************

	Konami 053260 PCM Sound Chip

*********************************************************/

#include "driver.h"
#include "k053260.h"

static struct K053260_channel_def {
	unsigned long		rate;
	unsigned long		size;
	unsigned long		start;
	unsigned long		bank;
	unsigned long		volume;
	int					play;
	unsigned long		pan;
	unsigned long		pos;
	int					loop;
	int					adpcm;
	int					nibble;
	int					adpcm_data;
} K053260_channel[4];

static struct K053260_chip_def {
	const struct K053260_interface	*intf;
	int								channel;
	int								mode;
	int								regs[0x30];
	unsigned char					*rom;
	int								rom_size;
} K053260_chip;

static int adpcm_lookup[] = {
	1,		2,		4,		8,		16,		32,		64,		128,
	-128,	-64,	-32,	-16,	-8,		-4,		-2,		-1
};

static unsigned long *delta_table;

static void InitDeltaTable( void ) {
	int		i;
	double	base = ( double )Machine->sample_rate;
	double	max = (double)K053260_chip.intf->clock; /* hz */
	unsigned long val;

	for( i = 0; i < 0x1000; i++ ) {
		double v = ( double )( 0x1000 - i );
		double target = max / v;

		if ( target && base ) {
			target = 65535.0 / ( base / target );
			val = ( unsigned long )target;
		} else
			val = 1;

		delta_table[i] = val;
	}
}

static void K053260_reset( void ) {
	int i;

	for( i = 0; i < 4; i++ ) {
		K053260_channel[i].rate = 0;
		K053260_channel[i].size = 0;
		K053260_channel[i].start = 0;
		K053260_channel[i].bank = 0;
		K053260_channel[i].volume = 0;
		K053260_channel[i].play = 0;
		K053260_channel[i].pan = 0;
		K053260_channel[i].pos = 0;
		K053260_channel[i].loop = 0;
		K053260_channel[i].adpcm = 0;
		K053260_channel[i].nibble = 0;
		K053260_channel[i].adpcm_data = 0;
	}
}

INLINE int limit( int val, int max, int min ) {
	if ( val > max )
		val = max;
	else if ( val < min )
		val = min;

	return val;
}

#define MAXOUT 0x7fff
#define MINOUT -0x8000

void K053260_update( int param, void **buffer, int length ) {
	int i, j, vol[4], val[4], pan[4], play[4], loop[4], nibble[4], adpcm_data[4], adpcm[4];
	unsigned char *rom[4];
	unsigned long delta[4], end[4], pos[4];
	unsigned char **buf = ( unsigned char ** )buffer;
	unsigned short **buf16 = ( unsigned short ** )buffer;
	int dataL, dataR;
	signed char d;

	/* precache some values */
	for ( i = 0; i < 4; i++ ) {
		rom[i]= &K053260_chip.rom[K053260_channel[i].start + ( K053260_channel[i].bank << 16 )];
		delta[i] = delta_table[K053260_channel[i].rate & 0x0fff];
		vol[i] = K053260_channel[i].volume;
		end[i] = K053260_channel[i].size;
		pos[i] = K053260_channel[i].pos;
		pan[i] = K053260_channel[i].pan;
		play[i] = K053260_channel[i].play;
		loop[i] = K053260_channel[i].loop;
		adpcm[i] = K053260_channel[i].adpcm;
		nibble[i] = K053260_channel[i].nibble;
		adpcm_data[i] = K053260_channel[i].adpcm_data;
	}

	/* 16 bit case */
	if ( Machine->sample_bits == 16 ) {
		for ( j = 0; j < length; j++ ) {

			dataL = dataR = 0;

			for ( i = 0; i < 4; i++ ) {
				val[i] = 0;
				/* see if the voice is on */
				if ( play[i] ) {
					/* see if we're done */
					if ( ( pos[i] >> 16 ) >= end[i] ) {
						if ( loop[i] )
							pos[i] = 0;
						else
							play[i] = 0;
						nibble[i] = adpcm_data[i] = 0;
					} else {
						if ( adpcm[i] ) { /* ADPCM */
							if ( nibble[i] ) {
								adpcm_data[i] += adpcm_lookup[ rom[i][pos[i] >> 16] & 0x0f ];
								pos[i] += delta[i];
							} else
								adpcm_data[i] += adpcm_lookup[ ( ( rom[i][pos[i] >> 16] ) >> 4 ) & 0x0f ];

							nibble[i] ^= 1;

							adpcm_data[i] /= 2; /* Normalize */

							dataL += ( adpcm_data[i] * vol[i] * pan[i] ) >> 2;
							dataR += ( adpcm_data[i] * vol[i] * ( 8 - pan[i] ) ) >> 2;
						} else { /* PCM */
							d = rom[i][pos[i] >> 16];

							dataL += ( d * vol[i] * pan[i] ) >> 2;
							dataR += ( d * vol[i] * ( 8 - pan[i] ) ) >> 2;

							pos[i] += delta[i];
						}
					}
				}
			}

			dataL = limit( dataL, MAXOUT, MINOUT );
			dataR = limit( dataR, MAXOUT, MINOUT );

			if ( K053260_chip.mode & 4 )
				buf16[0][j] = dataL;
			else
				buf16[0][j] = 0;

			if ( K053260_chip.mode & 2 )
				buf16[1][j] = dataR;
			else
				buf16[1][j] = 0;
		}
	} else { /* 8 bit case */
		for ( j = 0; j < length; j++ ) {

			dataL = dataR = 0;

			for ( i = 0; i < 4; i++ ) {
				val[i] = 0;
				/* see if the voice is on */
				if ( play[i] ) {
					/* see if we're done */
					if ( ( pos[i] >> 16 ) >= end[i] ) {
						if ( loop[i] )
							pos[i] = 0;
						else
							play[i] = 0;
						nibble[i] = adpcm_data[i] = 0;
					} else {
						if ( adpcm[i] ) { /* ADPCM */
							if ( nibble[i] ) {
								adpcm_data[i] += adpcm_lookup[ ( rom[i][pos[i] >> 16] ) & 0x0f ];
								pos[i] += delta[i];
							} else
								adpcm_data[i] += adpcm_lookup[ ( ( rom[i][pos[i] >> 16] ) >> 4 ) & 0x0f ];

							adpcm_data[i] /= 2; /* Normalize */

							nibble[i] ^= 1;

							dataL += ( adpcm_data[i] * vol[i] * pan[i] ) >> 2;
							dataR += ( adpcm_data[i] * vol[i] * ( 8 - pan[i] ) ) >> 2;
						} else { /* PCM */
							d = rom[i][pos[i] >> 16];

							dataL += ( d * vol[i] * pan[i] ) >> 2;
							dataR += ( d * vol[i] * ( 8 - pan[i] ) ) >> 2;

							pos[i] += delta[i];
						}
					}
				}
			}

			dataL = limit( dataL, MAXOUT, MINOUT );
			dataR = limit( dataR, MAXOUT, MINOUT );

			if ( K053260_chip.mode & 4 )
				buf[0][j] = dataL >> 8;
			else
				buf[0][j] = 0;

			if ( K053260_chip.mode & 2 )
				buf[1][j] = dataR >> 8;
			else
				buf[1][j] = 0;
		}
	}

	/* update the regs now */
	for ( i = 0; i < 4; i++ ) {
		K053260_channel[i].pos = pos[i];
		K053260_channel[i].play = play[i];
		K053260_channel[i].nibble = nibble[i];
		K053260_channel[i].adpcm_data = adpcm_data[i];
	}
}

int K053260_sh_start(const struct MachineSound *msound) {
	const char *names[2];
	char ch_names[2][40];
	int i;

	K053260_reset();

	for ( i = 0; i < 0x30; i++ )
		K053260_chip.regs[i] = 0;

	/* Initialize our chip structure */
	K053260_chip.intf = msound->sound_interface;
	K053260_chip.mode = 0;
	K053260_chip.rom = Machine->memory_region[K053260_chip.intf->region];
	K053260_chip.rom_size = Machine->memory_region_length[K053260_chip.intf->region] - 1;

	delta_table = ( unsigned long * )malloc( 0x1000 * sizeof( unsigned long ) );

	if ( delta_table == 0 )
		return -1;

	for ( i = 0; i < 2; i++ ) {
		names[i] = ch_names[i];
		sprintf(ch_names[i],"%s Ch %d",sound_name(msound),i);
	}

	K053260_chip.channel = stream_init_multi( 2, names,
						K053260_chip.intf->mixing_level, Machine->sample_rate,
						Machine->sample_bits, 0, K053260_update );

	InitDeltaTable();

    return 0;
}

void K053260_sh_stop( void ) {
	if ( delta_table )
		free( delta_table );
}

INLINE void check_bounds( int channel ) {
	int channel_start = ( K053260_channel[channel].bank << 16 ) + K053260_channel[channel].start;
	int channel_end = channel_start + K053260_channel[channel].size;

	if ( channel_start > K053260_chip.rom_size ) {
		if ( errorlog )
			fprintf( errorlog, "K53260: Attempting to start playing past the end of the rom ( start = %06x, end = %06x ).\n", channel_start, channel_end );

		K053260_channel[channel].play = 0;

		return;
	}

	if ( channel_end > K053260_chip.rom_size ) {
		if ( errorlog )
			fprintf( errorlog, "K53260: Attempting to play past the end of the rom ( start = %06x, end = %06x ).\n", channel_start, channel_end );

		K053260_channel[channel].size = K053260_chip.rom_size - channel_start;
	}
}

void K053260_WriteReg( int r,int v ) {
	int i, t;

	if ( r > 0x2f ) {
		if ( errorlog )
			fprintf( errorlog, "K053260: Writing past registers\n" );
		return;
	}

	if ( Machine->sample_rate != 0 )
		stream_update( K053260_chip.channel, 0 );

	/* before we update the regs, we need to check for a latched reg */
	if ( r == 0x28 ) {
		t = K053260_chip.regs[r] ^ v;

		for ( i = 0; i < 4; i++ ) {
			if ( t & ( 1 << i ) ) {
				if ( v & ( 1 << i ) ) {
					K053260_channel[i].play = 1;
					K053260_channel[i].pos = 0;
					K053260_channel[i].nibble = 0;
					K053260_channel[i].adpcm_data = 0;
					check_bounds( i );
				} else
					K053260_channel[i].play = 0;
			}
		}

		K053260_chip.regs[r] = v;
		return;
	}

	/* update regs */
	K053260_chip.regs[r] = v;

	/* communication registers */
	if ( r < 8 )
		return;

	/* channel setup */
	if ( r < 0x28 ) {
		int channel = ( r - 8 ) / 8;

		switch ( ( r - 8 ) & 0x07 ) {
			case 0: /* sample rate low */
				K053260_channel[channel].rate &= 0xff00;
				K053260_channel[channel].rate |= v;
			break;

			case 1: /* sample rate high */
				K053260_channel[channel].rate &= 0x00ff;
				K053260_channel[channel].rate |= v << 8;
			break;

			case 2: /* size low */
				K053260_channel[channel].size &= 0xff00;
				K053260_channel[channel].size |= v;
			break;

			case 3: /* size high */
				K053260_channel[channel].size &= 0x00ff;
				K053260_channel[channel].size |= v << 8;
			break;

			case 4: /* start low */
				K053260_channel[channel].start &= 0xff00;
				K053260_channel[channel].start |= v;
			break;

			case 5: /* start high */
				K053260_channel[channel].start &= 0x00ff;
				K053260_channel[channel].start |= v << 8;
			break;

			case 6: /* bank */
				K053260_channel[channel].bank = v & 0xff;
			break;

			case 7: /* volume is 7 bits. Convert to 8 bits now. */
				K053260_channel[channel].volume = ( v << 1 ) | ( v & 1 );
			break;
		}

		return;
	}

	switch( r ) {
		case 0x2a: /* loop, adpcm */
			for ( i = 0; i < 4; i++ )
				K053260_channel[i].loop = ( v & ( 1 << i ) ) != 0;

			for ( i = 4; i < 8; i++ )
				K053260_channel[i-4].adpcm = ( v & ( 1 << i ) ) != 0;
		break;

		case 0x2c: /* pan */
			K053260_channel[0].pan = v & 7;
			K053260_channel[1].pan = ( v >> 3 ) & 7;
		break;

		case 0x2d: /* more pan */
			K053260_channel[2].pan = v & 7;
			K053260_channel[3].pan = ( v >> 3 ) & 7;
		break;

		case 0x2f: /* control */
			K053260_chip.mode = v & 7;
			if ( v & 1 )
				K053260_reset();
		break;
	}
}

int K053260_ReadReg( int r ) {

	switch ( r ) {
		case 0x29: /* channel status */
			{
				int i, status = 0;

				for ( i = 0; i < 4; i++ )
					status |= K053260_channel[i].play << i;

				return status;
			}
		break;

		case 0x2e: /* read rom */
			if ( K053260_chip.mode & 1 ) {
				unsigned long offs = K053260_channel[0].start + ( K053260_channel[0].pos >> 16 ) + ( K053260_channel[0].bank << 16 );

				K053260_channel[0].pos += ( 1 << 16 );

				if ( offs > K053260_chip.rom_size ) {
					if ( errorlog )
						fprintf( errorlog, "K53260: Attempting to read past rom size on rom Read Mode.\n" );

					return 0;
				}

				return K053260_chip.rom[offs];
			}
		break;
	}

	return K053260_chip.regs[r];
}
