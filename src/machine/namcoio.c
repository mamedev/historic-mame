/***************************************************************************

Namco custom I/O chips 56XX and 58XX
(plus an unknown one used only by Pac & Pal - could be "57XX", I guess).
(56XX mode 7 is used only by liblrabl, it could be a different chip as well)

These chips work together with a 16XX, that interfaces them with the buffer
RAM. Each chip uses 16 nibbles of memory; the 16XX supports up to 4 chips,
but most games use only 2.

The 56XX and 58XX are pin-to-pin compatible, but not functionally equivalent:
they provide the same functions, but the command codes and memory addresses
are different, so they cannot be exchanged.

The devices have 42 pins. There are 16 input lines and 8 output lines to be
used for I/O.

It is very likely that the chips are standard 4-bit microcontrollers with
embedded ROM, but it hasn't been determined which type. The closest match
so far is the Oki MSM6408, but the pinout doesn't match 100%.


pin   description
---   -----------
1     clock (Mappy, Super Pac-Man)
2     clock (Gaplus; equivalent to the above?)
3     reset
4     irq
5-6   (to/from 16XX) (this is probably a normal I/O port used to synchronize with the 16XX)
7-8   ?
9-12  address to r/w from RAM; 12 also goes to the 16XX and acts as r/w line, so
      the chip can only read from addresses 0-7 and only write to addresses 8-F
	  (this is probably a normal I/O port used for that purpose)
13-16 out port A
17-20 out port B
21    GND
22-25 in port B
26-29 in port C
30-33 in port D
34-37 (to 16XX) probably data to r/w from RAM
	  (this is probably a normal I/O port used for that purpose)
38-41 in port A
42    Vcc

***************************************************************************/

#include "driver.h"
#include "machine/namcoio.h"

#define VERBOSE 0


data8_t namcoio_ram[MAX_NAMCOIO * 16];

struct namcoio
{
	int type;
	read8_handler in[4];
	write8_handler out[2];
	int reset;
	int lastcoins,lastbuttons;
	int credits;
	int coins[2];
	int coins_per_cred[2];
	int creds_per_coin[2];
};

static struct namcoio io[MAX_NAMCOIO];


static READ8_HANDLER( nop_r ) { return 0x0f; }
static WRITE8_HANDLER( nop_w ) { }

#define IORAM_READ(offset) (namcoio_ram[chip * 0x10 + (offset)] & 0x0f)
#define IORAM_WRITE(offset,data) {namcoio_ram[chip * 0x10 + (offset)] = (data) & 0x0f;}

#define READ_PORT(n)	(io[chip].in[n](0) & 0x0f)
#define WRITE_PORT(n,d)	io[chip].out[n](0,(d) & 0x0f)


static void handle_coins(int chip,int swap)
{
	int val,toggled;
	int credit_add = 0;
	int credit_sub = 0;
	int button;

//usrintf_showmessage("%x %x %x %x %x %x %x %x",IORAM_READ(8),IORAM_READ(9),IORAM_READ(10),IORAM_READ(11),IORAM_READ(12),IORAM_READ(13),IORAM_READ(14),IORAM_READ(15));

	val = ~READ_PORT(0);	// pins 38-41
	toggled = val ^ io[chip].lastcoins;
	io[chip].lastcoins = val;

	/* check coin insertion */
	if (val & toggled & 0x01)
	{
		io[chip].coins[0]++;
		if (io[chip].coins[0] >= (io[chip].coins_per_cred[0] & 7))
		{
			credit_add = io[chip].creds_per_coin[0] - (io[chip].coins_per_cred[0] >> 3);
			io[chip].coins[0] -= io[chip].coins_per_cred[0] & 7;
		}
		else if (io[chip].coins_per_cred[0] & 8)
			credit_add = 1;
	}
	if (val & toggled & 0x02)
	{
		io[chip].coins[1]++;
		if (io[chip].coins[1] >= (io[chip].coins_per_cred[1] & 7))
		{
			credit_add = io[chip].creds_per_coin[1] - (io[chip].coins_per_cred[1] >> 3);
			io[chip].coins[1] -= io[chip].coins_per_cred[1] & 7;
		}
		else if (io[chip].coins_per_cred[1] & 8)
			credit_add = 1;
	}
	if (val & toggled & 0x08)
	{
		credit_add = 1;
	}

	val = ~READ_PORT(3);	// pins 30-33
	toggled = val ^ io[chip].lastbuttons;
	io[chip].lastbuttons = val;

	/* check start buttons, only if the game allows */
	if (IORAM_READ(9) == 0)
	// the other argument is IORAM_READ(10) = 1, meaning unknown
	{
		if (val & toggled & 0x04)
		{
			if (io[chip].credits >= 1) credit_sub = 1;
		}
		if (val & toggled & 0x08)
		{
			if (io[chip].credits >= 2) credit_sub = 2;
		}
	}

	io[chip].credits += credit_add - credit_sub;

	IORAM_WRITE(0 ^ swap, io[chip].credits / 10);	// BCD credits
	IORAM_WRITE(1 ^ swap, io[chip].credits % 10);	// BCD credits
	IORAM_WRITE(2 ^ swap, credit_add);	// credit increment (coin inputs)
	IORAM_WRITE(3 ^ swap, credit_sub);	// credit decrement (start buttons)
	IORAM_WRITE(4, ~READ_PORT(1));	// pins 22-25
	button = ((val & 0x05) << 1) | (val & toggled & 0x05);
	IORAM_WRITE(5, button);	// pins 30 & 32 normal and impulse
	IORAM_WRITE(6, ~READ_PORT(2));	// pins 26-29
	button = (val & 0x0a) | ((val & toggled & 0x0a) >> 1);
	IORAM_WRITE(7, button);	// pins 31 & 33 normal and impulse
}



static void namco_customio_56XX_run(int chip)
{
#if VERBOSE
	logerror("execute 56XX %d mode %d\n",chip,IORAM_READ(8));
#endif

	switch (IORAM_READ(8))
	{
		case 0:	// nop?
			break;

		case 1:	// read switch inputs
			IORAM_WRITE(0, ~READ_PORT(0));	// pins 38-41
			IORAM_WRITE(1, ~READ_PORT(1));	// pins 22-25
			IORAM_WRITE(2, ~READ_PORT(2));	// pins 26-29
			IORAM_WRITE(3, ~READ_PORT(3));	// pins 30-33

//usrintf_showmessage("%x %x %x %x %x %x %x %x",IORAM_READ(8),IORAM_READ(9),IORAM_READ(10),IORAM_READ(11),IORAM_READ(12),IORAM_READ(13),IORAM_READ(14),IORAM_READ(15));

			WRITE_PORT(0,IORAM_READ(9));	// output to pins 13-16 (motos, pacnpal, gaplus)
			WRITE_PORT(1,IORAM_READ(10));	// output to pins 17-20 (gaplus)
			break;

		case 2:	// initialize coinage settings
			io[chip].coins_per_cred[0] = IORAM_READ(9);
			io[chip].creds_per_coin[0] = IORAM_READ(10);
			io[chip].coins_per_cred[1] = IORAM_READ(11);
			io[chip].creds_per_coin[1] = IORAM_READ(12);
			// IORAM_READ(13) = 1; meaning unknown - possibly a 3rd coin input? (there's a IPT_UNUSED bit in port A)
			// IORAM_READ(14) = 1; meaning unknown - possibly a 3rd coin input? (there's a IPT_UNUSED bit in port A)
			// IORAM_READ(15) = 0; meaning unknown
			break;

		case 4:	// druaga, digdug chip #1: read dip switches and inputs
				// superpac chip #0: process coin and start inputs, read switch inputs
			handle_coins(chip,0);
			break;

		case 7:	// bootup check (liblrabl only)
			{
				// liblrabl chip #1: 9-15 = f 1 2 3 4 0 0, expects 2 = e
				// liblrabl chip #2: 9-15 = 0 1 4 5 5 0 0, expects 7 = 6
				IORAM_WRITE(2,0xe);
				IORAM_WRITE(7,0x6);
			}
			break;

		case 8:	// bootup check
			{
				int i,sum;

				// superpac: 9-15 = f f f f f f f, expects 0-1 = 6 9. 0x69 = f+f+f+f+f+f+f.
				// motos:    9-15 = f f f f f f f, expects 0-1 = 6 9. 0x69 = f+f+f+f+f+f+f.
				// phozon:   9-15 = 1 2 3 4 5 6 7, expects 0-1 = 1 c. 0x1c = 1+2+3+4+5+6+7
				sum = 0;
				for (i = 9;i < 16;i++)
					sum += IORAM_READ(i);
				IORAM_WRITE(0,sum >> 4);
				IORAM_WRITE(1,sum & 0xf);
			}
			break;

		case 9:	// read dip switches and inputs
			WRITE_PORT(0,0);	// set pin 13 = 0
			IORAM_WRITE(0, ~READ_PORT(0));	// pins 38-41, pin 13 = 0
			IORAM_WRITE(2, ~READ_PORT(1));	// pins 22-25, pin 13 = 0
			IORAM_WRITE(4, ~READ_PORT(2));	// pins 26-29, pin 13 = 0
			IORAM_WRITE(6, ~READ_PORT(3));	// pins 30-33, pin 13 = 0
			WRITE_PORT(0,1);	// set pin 13 = 1
			IORAM_WRITE(1, ~READ_PORT(0));	// pins 38-41, pin 13 = 1
			IORAM_WRITE(3, ~READ_PORT(1));	// pins 22-25, pin 13 = 1
			IORAM_WRITE(5, ~READ_PORT(2));	// pins 26-29, pin 13 = 1
			IORAM_WRITE(7, ~READ_PORT(3));	// pins 30-33, pin 13 = 1
			break;

		default:
			logerror("Namco I/O %d: unknown I/O mode %d\n",chip,IORAM_READ(8));
	}
}



static void namco_customio_pacnpal_run(int chip)
{
#if VERBOSE
	logerror("execute PACNPAL %d mode %d\n",chip,IORAM_READ(8));
#endif

	switch (IORAM_READ(8))
	{
		case 0:	// nop?
			break;

		case 3:	// pacnpal chip #1: read dip switches and inputs
			IORAM_WRITE(4, ~READ_PORT(0));	// pins 38-41, pin 13 = 0 ?
			IORAM_WRITE(5, ~READ_PORT(2));	// pins 26-29 ?
			IORAM_WRITE(6, ~READ_PORT(1));	// pins 22-25 ?
			IORAM_WRITE(7, ~READ_PORT(3));	// pins 30-33
			break;

		default:
			logerror("Namco I/O %d: unknown I/O mode %d\n",chip,IORAM_READ(8));
	}
}



static void namco_customio_58XX_run(int chip)
{
#if VERBOSE
	logerror("execute 58XX %d mode %d\n",chip,IORAM_READ(8));
#endif

	switch (IORAM_READ(8))
	{
		case 0:	// nop?
			break;

		case 1:	// read switch inputs
			IORAM_WRITE(4, ~READ_PORT(0));	// pins 38-41
			IORAM_WRITE(5, ~READ_PORT(1));	// pins 22-25
			IORAM_WRITE(6, ~READ_PORT(2));	// pins 26-29
			IORAM_WRITE(7, ~READ_PORT(3));	// pins 30-33

//usrintf_showmessage("%x %x %x %x %x %x %x %x",IORAM_READ(8),IORAM_READ(9),IORAM_READ(10),IORAM_READ(11),IORAM_READ(12),IORAM_READ(13),IORAM_READ(14),IORAM_READ(15));

			WRITE_PORT(0,IORAM_READ(9));	// output to pins 13-16 (toypop)
			WRITE_PORT(1,IORAM_READ(10));	// output to pins 17-20 (toypop)
			break;

		case 2:	// initialize coinage settings
			io[chip].coins_per_cred[0] = IORAM_READ(9);
			io[chip].creds_per_coin[0] = IORAM_READ(10);
			io[chip].coins_per_cred[1] = IORAM_READ(11);
			io[chip].creds_per_coin[1] = IORAM_READ(12);
			// IORAM_READ(13) = 1; meaning unknown - possibly a 3rd coin input? (there's a IPT_UNUSED bit in port A)
			// IORAM_READ(14) = 0; meaning unknown - possibly a 3rd coin input? (there's a IPT_UNUSED bit in port A)
			// IORAM_READ(15) = 0; meaning unknown
			break;

		case 3:	// process coin and start inputs, read switch inputs
			handle_coins(chip,2);
			break;

		case 4:	// read dip switches and inputs
			WRITE_PORT(0,0);	// set pin 13 = 0
			IORAM_WRITE(0, ~READ_PORT(0));	// pins 38-41, pin 13 = 0
			IORAM_WRITE(2, ~READ_PORT(1));	// pins 22-25, pin 13 = 0
			IORAM_WRITE(4, ~READ_PORT(2));	// pins 26-29, pin 13 = 0
			IORAM_WRITE(6, ~READ_PORT(3));	// pins 30-33, pin 13 = 0
			WRITE_PORT(0,1);	// set pin 13 = 1
			IORAM_WRITE(1, ~READ_PORT(0));	// pins 38-41, pin 13 = 1
			IORAM_WRITE(3, ~READ_PORT(1));	// pins 22-25, pin 13 = 1
			IORAM_WRITE(5, ~READ_PORT(2));	// pins 26-29, pin 13 = 1
			IORAM_WRITE(7, ~READ_PORT(3));	// pins 30-33, pin 13 = 1
			break;

		case 5:	// bootup check
			/* mode 5 values are checked against these numbers during power up
			   mappy:  9-15 = 3 6 5 f a c e, expects 1-7 =   8 4 6 e d 9 d
			   grobda: 9-15 = 2 3 4 5 6 7 8, expects 2 = f and 6 = c
			   phozon: 9-15 = 0 1 2 3 4 5 6, expects 0-7 = 0 2 3 4 5 6 c a
			   gaplus: 9-15 = f f f f f f f, expects 0-1 = f f

			   This has been determined to be the result of repeated XORs,
			   controlled by a 7-bit LFSR. The following algorithm should be
			   equivalent to the original one (though probably less efficient).
			   The first nibble of the result however is uncertain. It is usually
			   0, but in some cases it toggles between 0 and F. We use a kludge
			   to give Gaplus the F it expects.
			*/
			{
				int i,n,rng,seed;
				#define NEXT(n) ((((n) & 1) ? (n) ^ 0x90 : (n)) >> 1)

				/* initialize the LFSR depending on the first two arguments */
				n = (IORAM_READ(9) * 16 + IORAM_READ(10)) & 0x7f;
				seed = 0x22;
				for (i = 0;i < n;i++)
					seed = NEXT(seed);

				/* calculate the answer */
				for (i = 1;i < 8;i++)
				{
					n = 0;
					rng = seed;
					if (rng & 1) { n ^= ~IORAM_READ(11); }
					rng = NEXT(rng);
					seed = rng;	// save state for next loop
					if (rng & 1) { n ^= ~IORAM_READ(10); }
					rng = NEXT(rng);
					if (rng & 1) { n ^= ~IORAM_READ(9); }
					rng = NEXT(rng);
					if (rng & 1) { n ^= ~IORAM_READ(15); }
					rng = NEXT(rng);
					if (rng & 1) { n ^= ~IORAM_READ(14); }
					rng = NEXT(rng);
					if (rng & 1) { n ^= ~IORAM_READ(13); }
					rng = NEXT(rng);
					if (rng & 1) { n ^= ~IORAM_READ(12); }

					IORAM_WRITE(i,~n);
				}
				IORAM_WRITE(0,0x0);
				/* kludge for gaplus */
				if (IORAM_READ(9) == 0xf) IORAM_WRITE(0,0xf);
			}
			break;

		default:
			logerror("Namco I/O %d: unknown I/O mode %d\n",chip,IORAM_READ(8));
	}
}



READ8_HANDLER( namcoio_r )
{
	// RAM is 4-bit wide; Pac & Pal requires the | 0xf0 otherwise Easter egg doesn't work
	offset &= 0x3f;

#if VERBOSE
	logerror("%04x: I/O read %d: mode %d, offset %d = %02x\n", activecpu_get_pc(), offset / 16, namcoio_ram[(offset & 0x30) + 8], offset & 0x0f, namcoio_ram[offset]&0x0f);
#endif

	return 0xf0 | namcoio_ram[offset];
}

WRITE8_HANDLER( namcoio_w )
{
	offset &= 0x3f;
	data &= 0x0f;	// RAM is 4-bit wide

#if VERBOSE
	logerror("%04x: I/O write %d: offset %d = %02x\n", activecpu_get_pc(), offset / 16, offset & 0x0f, data);
#endif

	namcoio_ram[offset] = data;
}

void namcoio_set_reset_line(int chipnum, int state)
{
	io[chipnum].reset = (state == ASSERT_LINE) ? 1 : 0;
	if (state != CLEAR_LINE)
	{
		/* reset internal registers */
		io[chipnum].credits = 0;
		io[chipnum].coins[0] = 0;
		io[chipnum].coins_per_cred[0] = 1;
		io[chipnum].creds_per_coin[0] = 1;
		io[chipnum].coins[1] = 0;
		io[chipnum].coins_per_cred[1] = 1;
		io[chipnum].creds_per_coin[1] = 1;
	}
}

static void namcoio_run(int param)
{
	switch (io[param].type)
	{
		case NAMCOIO_56XX:
			namco_customio_56XX_run(param);
			break;
		case NAMCOIO_58XX:
			namco_customio_58XX_run(param);
			break;
		case NAMCOIO_PACNPAL:
			namco_customio_pacnpal_run(param);
			break;
	}
}

void namcoio_set_irq_line(int chipnum, int state)
{
	if (chipnum < MAX_NAMCOIO && state != CLEAR_LINE && !io[chipnum].reset)
	{
		/* give the cpu a tiny bit of time to write the command before processing it */
		timer_set(TIME_IN_USEC(50), chipnum, namcoio_run);
	}
}

void namcoio_init(int chipnum, int type, const struct namcoio_interface *intf)
{
	if (chipnum < MAX_NAMCOIO)
	{
		io[chipnum].type = type;
		io[chipnum].in[0] = intf->in[0] ? intf->in[0] : nop_r;
		io[chipnum].in[1] = intf->in[1] ? intf->in[1] : nop_r;
		io[chipnum].in[2] = intf->in[2] ? intf->in[2] : nop_r;
		io[chipnum].in[3] = intf->in[3] ? intf->in[3] : nop_r;
		io[chipnum].out[0] = intf->out[0] ? intf->out[0] : nop_w;
		io[chipnum].out[1] = intf->out[1] ? intf->out[1] : nop_w;
		namcoio_set_reset_line(chipnum,PULSE_LINE);
	}
}
